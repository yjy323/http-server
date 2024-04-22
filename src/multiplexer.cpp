#include "multiplexer.hpp"

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "core.hpp"

static const bool REUSABLE = false;
static const int BACKLOG = 128;
static const int EVENT_SIZE = 30;
static const int POLLING_TIMEOUT = 120;
static const int EVENT_TIMEOUT = 10;
static const int CGI_EVENT_TIMEOUT = 5;
static const int KEEP_ALIVE_TIMEOUT = 10;
static const int SEND_EVENT_TIMEOUT = 3;
static int SERVER_UDATA = -2;
static int CLIENT_UDATA = -3;

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
      continue;
    }
    HandleEvents(nev);
  }
  return OK;
}

void Multiplexer::HandleEvents(int nev) {
  struct kevent* events = eh_.events();

  for (int i = 0; i < nev; ++i) {
    if (events[i].flags | EV_ERROR && !(events[i].filter & EVFILT_PROC)) {
      HandleErrorEvent(events[i]);
    } else if (events[i].filter == EVFILT_READ) {
      HandleReadEvent(events[i]);
    } else if (events[i].filter == EVFILT_WRITE) {
      HandleWriteEvent(events[i]);
    } else if (events[i].filter == EVFILT_PROC) {
      HandleCgiEvent(events[i]);
    } else if (events[i].filter == EVFILT_TIMER) {
      HandleTimeoutEvent(events[i]);
    } else {
    }
  }
}

void Multiplexer::HandleErrorEvent(struct kevent& event) {
  if (clients_.find(event.ident) != clients_.end()) {
    Client& client = *clients_[event.ident];
    DisconnetClient(client);
  }
}

void Multiplexer::HandleReadEvent(struct kevent& event) {
  if (*static_cast<int*>(event.udata) == SERVER_UDATA) {
    AcceptWithClient(event.ident);
  } else if (*static_cast<int*>(event.udata) == CLIENT_UDATA) {
    Client& client = *clients_[event.ident];

    if (client.ReceiveRequest() <= 0) {
      DisconnetClient(client);
      return;
    }

    if (client.IsReceiveRequestHeaderComplete()) {
      int http_status = client.ParseRequestHeader();

      if (http_status != HTTP_OK) {
        client.CreateResponseMessage();
        if (this->eh_.AddWithTimer(client.fd(), EVFILT_WRITE, EV_ONESHOT, 0, 0,
                                   &CLIENT_UDATA,
                                   SEND_EVENT_TIMEOUT) == ERROR) {
          DisconnetClient(client);
          return;
        }
      } else {
        if (client.IsReceiveRequestBodyComplete()) {
          if (client.transaction().status_code() != HTTP_OK ||
              client.ParseRequestBody() != HTTP_OK) {
            client.CreateResponseMessage();
            if (this->eh_.AddWithTimer(client.fd(), EVFILT_WRITE, EV_ONESHOT, 0,
                                       0, &CLIENT_UDATA,
                                       SEND_EVENT_TIMEOUT) == ERROR) {
              DisconnetClient(client);
              return;
            }
          }
          client.transaction().HttpProcess();
          if (client.transaction().cgi().on()) {
            pid_t pid = client.transaction().ExecuteCgi();
            if (pid > 0 &&
                eh_.AddWithTimer(client.transaction().cgi().server2cgi_fd()[1],
                                 EVFILT_WRITE, EV_ONESHOT, NOTE_EXIT, 0,
                                 &client.fd(), CGI_EVENT_TIMEOUT) == ERROR) {
              DisconnetClient(client);
              return;
            }
            return;
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
  } else if (clients_.find(*static_cast<int*>(event.udata)) != clients_.end()) {
    Client& client = *clients_[*static_cast<int*>(event.udata)];

    client.CreateResponseMessageByCgi();
    eh_.Delete(client.transaction().cgi().pid(), EVFILT_TIMER);
    client.transaction().cgi().set_pid(-1);

    if (eh_.Add(client.fd(), EVFILT_WRITE, EV_ONESHOT, 0, 0, &CLIENT_UDATA) ==
        ERROR) {
      DisconnetClient(client);
    }
  }
}

void Multiplexer::HandleWriteEvent(struct kevent& event) {
  if (*static_cast<int*>(event.udata) == CLIENT_UDATA) {
    Client& client = *clients_[event.ident];

    if (client.SendResponseMessage() == -1) {
      return (void)DisconnetClient(client);
    }

    if (!client.transaction().headers_out().connection_close) {
      if (this->eh_.Add(client.fd(), EVFILT_TIMER, EV_ONESHOT, 0,
                        KEEP_ALIVE_TIMEOUT * 1000, &CLIENT_UDATA) == ERROR) {
        return (void)DisconnetClient(client);
      }
      client.ResetClientInfo();
    } else {
      return (void)DisconnetClient(client);
    }
  } else if (clients_.find(*static_cast<int*>(event.udata)) != clients_.end()) {
    Client& client = *clients_[*static_cast<int*>(event.udata)];

    int server2cgi_fd_out = client.transaction().cgi().server2cgi_fd()[1];
    std::string form_data_str = client.transaction().body_in();
    const char* form_data = form_data_str.c_str();

    eh_.Delete(server2cgi_fd_out, EVFILT_TIMER);

    size_t len = std::strlen(form_data);
    while (len > 0) {
      ssize_t chunk_size = write(server2cgi_fd_out, form_data, len);
      if (chunk_size < 0) {
        client.transaction().set_status_code(HTTP_INTERNAL_SERVER_ERROR);
        client.transaction().CreateResponseMessage();
        if (eh_.Add(client.fd(), EVFILT_WRITE, EV_ONESHOT, 0, 0,
                    &CLIENT_UDATA) == ERROR) {
          DisconnetClient(client);
          return;
        }
      }

      form_data += chunk_size;
      len -= chunk_size;
    }
    close(server2cgi_fd_out);
    client.transaction().cgi().set_server2cgi_fd(1, -1);

    pid_t pid = client.transaction().cgi().pid();
    if (eh_.AddWithTimer(pid, EVFILT_PROC, EV_ONESHOT, NOTE_EXIT, 0,
                         &client.fd(), CGI_EVENT_TIMEOUT) == ERROR) {
      DisconnetClient(client);
      return;
    }
  }
}

void Multiplexer::HandleCgiEvent(struct kevent& event) {
  if (clients_.find(*static_cast<int*>(event.udata)) == clients_.end()) return;
  Client& client = *clients_[*static_cast<int*>(event.udata)];

  eh_.Delete(event.ident, EVFILT_TIMER);
  if (event.flags & EV_ERROR) {
    client.transaction().set_status_code(HTTP_BAD_GATEWAY);
  }
  if (eh_.AddWithTimer(client.transaction().cgi().cgi2server_fd()[0],
                       EVFILT_READ, EV_ONESHOT, 0, 0, &client.fd(),
                       CGI_EVENT_TIMEOUT) == ERROR) {
    DisconnetClient(client);
  }
}

void Multiplexer::HandleTimeoutEvent(struct kevent& event) {
  if (*static_cast<int*>(event.udata) == SERVER_UDATA) {
    return;
  } else if (*static_cast<int*>(event.udata) == CLIENT_UDATA) {
    if (clients_.find(event.ident) == clients_.end()) return;
    Client& client = *clients_[event.ident];

    DisconnetClient(client);
  } else if (clients_.find(*static_cast<int*>(event.udata)) != clients_.end()) {
    Client& client = *clients_[*static_cast<int*>(event.udata)];
    if ((client.transaction().cgi().server2cgi_fd()[1] !=
         static_cast<int>(event.ident)) &&
        (client.transaction().cgi().pid() != static_cast<pid_t>(event.ident))) {
      return;
    }
    client.transaction().set_status_code(HTTP_GATEWAY_TIME_OUT);
    client.CreateResponseMessage();
    client.transaction().cgi().set_pid(-1);
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

  return OK;
}

void Multiplexer::DisconnetClient(Client& client) {
  if (client.transaction().cgi().pid() > 0) {
    eh_.Delete(client.transaction().cgi().pid());
    eh_.Delete(client.transaction().cgi().pid(), EVFILT_TIMER);
  }
  clients_.erase(client.fd());
  eh_.Delete(client.fd(), EVFILT_TIMER);
  eh_.Delete(client.fd());
  delete &client;
}
