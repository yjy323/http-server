#include "multiplexer.hpp"

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "core.hpp"

static const bool REUSABLE = false;
static const int BACKLOG = 128;
static const int EVENT_SIZE = 30;
static const int POLLING_TIMEOUT = 1000;
static const int EVENT_TIMEOUT = 3000;
static const int CGI_EVENT_TIMEOUT = 1000;
static const int KEEP_ALIVE_TIMEOUT = 5;
static const int SEND_EVENT_TIMEOUT = 1000;
static int SERVER_UDATA = (1 << 0);
static int CLIENT_UDATA = (1 << 1);

Multiplexer::Multiplexer() : servers_(), clients_(), eh_() {}

Multiplexer::~Multiplexer() {
  for (Servers::iterator it = servers_.begin(); it != servers_.end(); it++)
    if (it->second != 0) delete it->second;
  for (Clients::iterator it = clients_.begin(); it != clients_.end(); it++)
    if (it->second != 0) delete it->second;
}

int Multiplexer::Init(const Configuration& config) {
  servers_.clear();

  if (InitServers(config) == ERROR || eh_.Init(EVENT_SIZE) == ERROR)
    return ERROR;

  return OK;
}

int Multiplexer::StartServers() {
  for (Servers::const_iterator it = servers_.begin(); it != servers_.end();
       it++) {
    if (Multiplexer::StartServer(*(it->second)) == ERROR) {
      return ERROR;
    }
  }

  return 0;
}

int Multiplexer::StartServer(Server& server) {
  int opt_val = 1;
  if (REUSABLE || server.Setsockopt(SOL_SOCKET, SO_REUSEADDR, &opt_val,
                                    sizeof(opt_val)) == -1) {
    return ERROR;
  }

  if (server.Bind() == -1 || server.Listen(BACKLOG) == -1 ||
      server.SetNonBlock() == -1) {
    return ERROR;
  }

  if (eh_.Add(server.fd(), EVFILT_READ, 0, 0, 0,
              static_cast<void*>(&SERVER_UDATA)) == ERROR) {
    return ERROR;
  }

  return OK;
}

int Multiplexer::InitServers(const Configuration& config) {
  std::map<int, Server::Configurations> configs_by_port;

  for (Configuration::const_iterator it = config.begin(); it != config.end();
       it++) {
    if (configs_by_port.find(it->port()) == configs_by_port.end()) {
      configs_by_port[it->port()] = Server::Configurations();
    }
    configs_by_port[it->port()].push_back(*it);
  }

  for (std::map<int, Server::Configurations>::iterator it =
           configs_by_port.begin();
       it != configs_by_port.end(); it++) {
    if (InitServer(it->second) == ERROR) {
      return ERROR;
    }
  }

  return OK;
}

int Multiplexer::InitServer(const Server::Configurations& configs) {
  Socket::Info info;
  info.domain = AF_INET;
  info.type = SOCK_STREAM;
  info.protocol = 0;

  Server* server = new Server(info, configs);
  if (server->fd() == -1) {
    return ERROR;
  }

  servers_[server->fd()] = server;

  return OK;
}

int Multiplexer::OpenServers() {
  while (1) {
    int nev;

    if (eh_.Polling(nev, POLLING_TIMEOUT) == ERROR) {
      std::cout << "polling error" << std::endl;
      continue;
    }
    std::cout << "nev: " << nev << std::endl;
    HandleEvents(nev);
  }
  return OK;
}

void Multiplexer::HandleEvents(int nev) {
  struct kevent* events = eh_.events();

  for (int i = 0; i < nev; ++i) {
    std::cout << "[event ident] " << events[i].ident << std::endl;
    if (events[i].flags | EV_ERROR && !(events[i].filter & EVFILT_PROC)) {
      std::clog << "[event type] ERROR" << std::endl;
      HandleErrorEvent(events[i]);
    } else if (events[i].filter == EVFILT_READ) {
      std::clog << "[event type] READ" << std::endl;
      HandleReadEvent(events[i]);
    } else if (events[i].filter == EVFILT_WRITE) {
      std::clog << "[event type] WRITE" << std::endl;
      HandleWriteEvent(events[i]);
      // clients_[events[i].ident].ResetClientTransactionInfo();
    } else if (events[i].filter == EVFILT_PROC) {
      std::clog << "[event type] PROC" << std::endl;
      HandleCgiEvent(events[i]);
    } else if (events[i].filter == EVFILT_TIMER) {
      std::clog << "[event type] TIMER" << std::endl;
      HandleTimeoutEvent(events[i]);
    } else {
      std::cerr << "UNDEFINE_FILTER_ERROR_MASSAGE" << std::endl;
    }
  }
}

void Multiplexer::HandleErrorEvent(struct kevent& event) {
  Client& client = *clients_[event.ident];
  DisconnetClient(client);
}

void Multiplexer::HandleReadEvent(struct kevent& event) {
  if (*static_cast<int*>(event.udata) == SERVER_UDATA) {
    AcceptWithClient(event.ident);
  } else if (*static_cast<int*>(event.udata) == CLIENT_UDATA) {
    Client& client = *clients_[event.ident];

    if (this->ReceiveRequest(client) == ERROR) {
      DisconnetClient(client);
      return;
    }

    if (client.IsReceiveRequestHeaderComplete()) {
      int http_status = client.ParseRequestHeader();

      if (http_status != HTTP_OK) {
        client.transaction().HttpProcess();
        client.CreateResponseMessage();
        if (this->eh_.AddWithTimer(client.fd(), EVFILT_WRITE, EV_ONESHOT, 0, 0,
                                   &CLIENT_UDATA,
                                   SEND_EVENT_TIMEOUT) == ERROR) {
          DisconnetClient(client);
          return;
        }
      } else {
        if (client.IsReceiveRequestBodyComplete()) {
          std::cout << "[Request Start]" << std::endl;
          std::cout << client.request() << std::endl;
          std::cout << "[Request End]" << std::endl;
          client.transaction().HttpProcess();
          if (client.transaction().cgi().on()) {
            pid_t pid = client.transaction().ExecuteCgi();
            if (pid > 0 && eh_.AddWithTimer(pid, EVFILT_PROC, EV_ONESHOT, 0, 0,
                                            static_cast<void*>(&client.fd()),
                                            CGI_EVENT_TIMEOUT) == ERROR) {
              return;
            }
          }

          client.CreateResponseMessage();
          if (this->eh_.AddWithTimer(client.fd(), EVFILT_WRITE, EV_ONESHOT, 0,
                                     0, &CLIENT_UDATA,
                                     SEND_EVENT_TIMEOUT) == ERROR) {
            DisconnetClient(client);
            return;
          }
        }
      }
    }
  } else {
  }
}

void Multiplexer::HandleWriteEvent(struct kevent& event) {
  Client& client = *clients_[event.ident];

  std::cout << "[Response Message Start]" << std::endl;
  std::cout << client.response() << std::endl;
  std::cout << "[Response Message End]" << std::endl;

  if (client.SendResponseMessage() == -1 ||
      this->eh_.Add(client.fd(), EVFILT_TIMER, EV_ONESHOT, 0,
                    KEEP_ALIVE_TIMEOUT * 1000, &CLIENT_UDATA) == ERROR) {
    DisconnetClient(client);
  }

  client.ResetClientInfo();
}

void Multiplexer::HandleCgiEvent(struct kevent& event) {
  Client& client = *clients_[*reinterpret_cast<int*>(event.udata)];
  if (event.flags & EV_ERROR) {
    client.transaction().set_status_code(HTTP_BAD_GATEWAY);
  }
  client.CreateResponseMessageByCgi();

  if (eh_.Add(event.ident, EVFILT_WRITE, EV_ONESHOT, 0, 0, &CLIENT_UDATA) ==
      ERROR) {
    DisconnetClient(client);
  }
}

void Multiplexer::HandleTimeoutEvent(struct kevent& event) {
  if (*static_cast<int*>(event.udata) == SERVER_UDATA) {
  } else if (*static_cast<int*>(event.udata) == CLIENT_UDATA) {
    Client& client = *clients_[event.ident];

    DisconnetClient(client);
  } else {
    Client& client = *clients_[*static_cast<int*>(event.udata)];

    if (eh_.Add(client.fd(), EVFILT_WRITE, EV_ONESHOT, 0, 0, &CLIENT_UDATA) ==
        ERROR) {
      DisconnetClient(client);
    }
  }
}

int Multiplexer::AcceptWithClient(int server_fd) {
  Server& server = *servers_[server_fd];

  int client_fd = server.Accept();
  if (client_fd == -1) return ERROR;
  clients_[client_fd] = new Client(server, client_fd);
  clients_[client_fd]->SetNonBlock();

  if (eh_.AddWithTimer(client_fd, EVFILT_READ, 0, 0, 0, &CLIENT_UDATA,
                       EVENT_TIMEOUT) == ERROR)
    return ERROR;

  std::cout << "Successful Accept With " << client_fd << std::endl;
  return OK;
}

void Multiplexer::DisconnetClient(Client& client) {
  delete &client;
  clients_.erase(client.fd());
  eh_.Delete(client.fd());
}

int Multiplexer::ReceiveRequest(Client& client) {
  ssize_t bytes_read = client.ReceiveRequest();

  if (bytes_read == -1) {
    return ERROR;
  }
  return OK;
}
