#include "multiplexer.hpp"

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "core.hpp"

#define IS_REUSABLE true
#define DEFAULT_BACKLOG 128
#define EVENT_SIZE 30
#define BUFFER_SIZE 2048
#define EVENT_POLL_TIME 3
#define READ_EVENT_TIMEOUT_SEC 3
#define KEEP_ALIVE_TIMEOUT_SEC 5
#define SERVER_UDATA (1 << 0)
#define CLIENT_UDATA (1 << 1)
#define CGI_UDATA (1 << 1)

#define REQUEST_HEADER_END CRLF CRLF

#define KQUEUE_ERROR_MASSAGE "kqueue() failed."
#define KEVENT_ERROR_MASSAGE "kevent() failed."
#define RECV_ERROR_MASSAGE "recv() from client failed."
#define ACCEPT_ERROR_MASSAGE "accept() from client failed."
#define UNDEFINE_FILTER_ERROR_MASSAGE "catch undefine filter."
#define NMANAGE_CLIENT_ERROR_MASSAGE \
  "A message request to a client that is not being managed."
#define REQUEST_HEADER_PARSE_ERROR_MASSAGE \
  "An error occurred while parsing header."

Multiplexer::Multiplexer()
    : server_udata_(SERVER_UDATA),
      client_udata_(CLIENT_UDATA),
      cgi_udata_(CGI_UDATA),
      servers_(),
      clients_(),
      eh_() {}

Multiplexer::~Multiplexer() {}

int Multiplexer::Init(const Configuration& configuration) {
  this->servers_.clear();

  if (InitConfiguration(configuration) == ERROR || InitServer() == ERROR ||
      this->eh_.Init(EVENT_SIZE) == ERROR)
    return ERROR;

  return OK;
}

int Multiplexer::InitConfiguration(const Configuration& configuration) {
  for (Configuration::const_iterator it = configuration.begin();
       it != configuration.end(); it++) {
    AddConfInServers(*it);
  }

  return OK;
}

int Multiplexer::InitServer() {
  for (std::vector<Server>::iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->Open() == ERROR || (IS_REUSABLE && it->SetReusable() == ERROR) ||
        it->Bind() == ERROR || it->Listen(DEFAULT_BACKLOG) == ERROR)
      return ERROR;

    std::clog << "server[ fd: " << it->fd()
              << " ] open 완료 (open, bind, listen)" << std::endl;
  }

  return OK;
}

int Multiplexer::Multiplexing() {
  for (std::vector<Server>::const_iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (this->eh_.Regist(it->fd(), EVFILT_READ, 0, 0, 0,
                         static_cast<void*>(&this->server_udata_)) == ERROR)
      return ERROR;

    std::clog << "server[ fd: " << it->fd() << " ] read event 등록 완료"
              << std::endl;
  }

  return StartServer();
}

int Multiplexer::StartServer() {
  while (1) {
    int nev;

    std::clog << "event 대기상태 진입" << std::endl;
    if (this->eh_.Polling(nev, EVENT_POLL_TIME) == ERROR) continue;
    HandleEvents(nev);
  }
}

void Multiplexer::HandleEvents(int nev) {
  struct kevent* events = this->eh_.events();

  for (int i = 0; i < nev; ++i) {
    if (events[i].filter == EVFILT_READ) {
      std::clog << "[event type] READ" << std::endl;
      HandleReadEvent(events[i]);
    } else if (events[i].filter == EVFILT_WRITE) {
      std::clog << "[event type] WRITE" << std::endl;
      HandleWriteEvent(events[i]);
      this->clients_[events[i].ident].ResetClientTransactionInfo();
    } else if (events[i].filter == EVFILT_PROC) {
      std::clog << "[event type] PROC" << std::endl;
      HandleCgiEvent(events[i]);
    } else if (events[i].filter == EVFILT_TIMER) {
      std::clog << "[event type] TIMER" << std::endl;
      eh_.DeleteAll(events[i].ident);
      CloseWithClient(events[i].ident);
    } else {
      std::cerr << UNDEFINE_FILTER_ERROR_MASSAGE << std::endl;
    }
  }
}

void Multiplexer::HandleReadEvent(struct kevent event) {
  if (*static_cast<int*>(event.udata) == server_udata_) {
    std::clog << "[READ event sub type] connection" << std::endl;
    AcceptWithClient(event.ident);
  } else if (*static_cast<int*>(event.udata) == client_udata_) {
    std::clog << "[READ event sub type] message" << std::endl;
    Client& client = this->clients_[event.ident];

    std::string buffer;
    if (ReadClientMessage(client.fd(), buffer) == ERROR) return;

    std::clog << "server[ " << client.server().fd()
              << " ]: 메세지 from client[ " << client.fd() << " ]" << std::endl;

    client.set_request_str(client.request_str() + buffer);

    if (size_t header_end = client.request_str().find(REQUEST_HEADER_END) !=
                            std::string::npos) {
      std::clog << " [ Request Message start ]  " << std::endl;
      std::clog << client.request_str() << std::endl;
      std::clog << " [ Request Message end ]  " << std::endl;

      int http_status = client.transaction_instance().ParseRequestHeader(
          client.request_str());

      const ServerConfiguration& sc =
          client.server().ConfByHost(client.transaction().headers_in().host);
      client.transaction_instance().uri().ReconstructTargetUri(
          client.transaction().headers_in().host);
      client.transaction_instance().GetConfiguration(sc);

      if (http_status != HTTP_OK) {
        std::cerr << REQUEST_HEADER_PARSE_ERROR_MASSAGE << std::endl;

        std::clog << "[ Parse Error Request ]" << std::endl
                  << client.request_str() << std::endl;
      } else {
        std::string request_body = client.request_str().substr(
            client.request_str().find(REQUEST_HEADER_END) +
            std::strlen(REQUEST_HEADER_END));
        if (IsReadFullBody(client, request_body)) {
          client.transaction_instance().HttpProcess();
          if (client.transaction().cgi().on()) {
            pid_t pid = client.transaction_instance().ExecuteCgi();
            if (pid > 0 &&
                this->eh_.Regist(pid, EVFILT_PROC, EV_ONESHOT, 0, 0,
                                 static_cast<void*>(&this->client_udata_)) ==
                    ERROR) {
              CloseWithClient(client.fd());
              this->eh_.DeleteAll(client.fd());

              return;
            }
          }
          client.set_response_str(
              client.transaction_instance().CreateResponseMessage());
          if (this->eh_.Regist(event.ident, EVFILT_WRITE, EV_ONESHOT, 0, 0,
                               static_cast<void*>(&this->client_udata_)) ==
              ERROR) {
            CloseWithClient(client.fd());
            this->eh_.DeleteAll(client.fd());

            return;
          }
        }
      }
    }
  } else {
    std::cerr << NMANAGE_CLIENT_ERROR_MASSAGE << std::endl;
  }
}

void Multiplexer::HandleWriteEvent(struct kevent event) {
  Client& client = this->clients_[event.ident];

  std::clog << std::endl << " [ Response Start ] " << std::endl;

  std::clog << client.response_str() << std::endl;
  std::clog << std::endl << " [ Response End ] " << std::endl;

  if (send(client.fd(), client.response_str().c_str(),
           client.response_str().length(), 0) == -1) {
    CloseWithClient(client.fd());
    this->eh_.DeleteAll(client.fd());
  }

  if (this->eh_.Regist(client.fd(), EVFILT_TIMER, EV_ONESHOT, 0,
                       KEEP_ALIVE_TIMEOUT_SEC * 1000,
                       static_cast<void*>(&this->client_udata_)) == ERROR) {
    CloseWithClient(client.fd());
    this->eh_.DeleteAll(client.fd());
  }
}

void Multiplexer::HandleCgiEvent(struct kevent event) {
  Client& client = this->clients_[event.ident];
  const Cgi& cgi = client.transaction_instance().cgi();
  std::string cgi_res;
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  while ((bytes_read = read(cgi.cgi2server_fd()[1], buffer, BUFFER_SIZE - 1)) >
         0) {
    buffer[BUFFER_SIZE - 1] = 0;

    cgi_res += buffer;
  }
  if (bytes_read == -1) {
    client.transaction_instance().set_status_code(HTTP_BAD_GATEWAY);
  } else {
    client.transaction_instance().entity().ReadBuffer(cgi_res.c_str(),
                                                      cgi_res.length());
    client.transaction_instance().CreateResponseMessage();
  }

  if (this->eh_.Regist(event.ident, EVFILT_WRITE, EV_ONESHOT, 0, 0,
                       static_cast<void*>(&this->client_udata_)) == ERROR) {
    CloseWithClient(client.fd());
    this->eh_.DeleteAll(client.fd());
  }
}

int Multiplexer::ReadClientMessage(int client_fd, std::string& message) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

  if (bytes_read == -1) {
    CloseWithClient(client_fd);

    this->eh_.DeleteAll(client_fd);

    std::cerr << RECV_ERROR_MASSAGE << std::endl;

    return ERROR;
  }

  buffer[bytes_read] = 0;
  message = buffer;

  return OK;
}

int Multiplexer::AcceptWithClient(int server_fd) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  int client_fd =
      accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
  if (client_fd == -1) {
    std::cerr << ACCEPT_ERROR_MASSAGE << std::endl;

    return ERROR;
  }

  for (std::vector<Server>::const_iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->fd() == server_fd) {
      this->clients_[client_fd] = Client(*it, client_fd);
      break;
    }
  }

  if (this->eh_.Regist(client_fd, EVFILT_READ, 0, 0, 0,
                       static_cast<void*>(&this->client_udata_)) == ERROR)
    return ERROR;
  std::clog << "client[ fd: " << client_fd << " ] read event 등록 완료"
            << std::endl;

  if (this->eh_.Regist(client_fd, EVFILT_TIMER, EV_ONESHOT, 0,
                       READ_EVENT_TIMEOUT_SEC * 1000,
                       static_cast<void*>(&this->client_udata_)) == ERROR)
    return ERROR;

  std::clog << "client[ fd: " << client_fd << "] 성공적으로 connect"
            << std::endl;

  return OK;
}

int Multiplexer::CloseWithClient(int client_fd) {
  close(client_fd);
  this->clients_.erase(client_fd);

  std::clog << "closed with [fd: " << client_fd << "]" << std::endl;

  return OK;
}

bool Multiplexer::IsReadFullBody(Client& client,
                                 const std::string& request_body) {
  const HeadersIn& headers_in = client.transaction().headers_in();
  const ServerConfiguration::LocationConfiguration& config =
      client.transaction().config();

  if (IsRequestBodyAllowed(client.transaction().method())) {
    return true;
  }

  if (headers_in.content_length_n >= 0) {
    if (request_body.length() >
        static_cast<size_t>(config.client_max_body_size())) {
      client.transaction_instance().set_status_code(
          HTTP_REQUEST_ENTITY_TOO_LARGE);
    } else if (request_body.length() <
               static_cast<size_t>(headers_in.content_length_n)) {
      return false;
    }
  } else if (headers_in.transfer_encoding == "chunked") {
    if (request_body.find("0" CRLF) == std::string::npos) {
      return false;
    }
  } else {
    if (request_body.length() > 0) {
      client.transaction_instance().set_status_code(HTTP_LENGTH_REQUIRED);
    }
  }

  return true;
}

void Multiplexer::AddConfInServers(const ServerConfiguration& server_conf) {
  if (IsExistPort(server_conf.port())) {
    ServerInstanceByPort(server_conf.port()).AddConf(server_conf);
  } else {
    Server server = Server();
    server.AddConf(server_conf);

    this->servers_.push_back(server);
  }
}

bool Multiplexer::IsExistPort(int port) {
  for (std::vector<Server>::const_iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->port() == port) return true;
  }

  return false;
}

bool Multiplexer::IsRequestBodyAllowed(const std::string& method) {
  if (method == "GET" || method == "DELETE") return false;

  return true;
}

Server& Multiplexer::ServerInstanceByPort(int port) {
  for (std::vector<Server>::iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->port() == port) {
      return *it;
    }
  }

  return *this->servers_.end();
}
