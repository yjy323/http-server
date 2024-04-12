#include "multiplexer.hpp"

#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "core.hpp"

#define IS_REUSABLE true
#define DEFAULT_BACKLOG 128
#define KEVENT_SIZE 10
#define BUFFER_SIZE 2048
#define EVENT_TIMEOUT_SEC 2
#define SERVER_IDENTIFIER (1 << 0)
#define CLIENT_IDENTIFIER (1 << 1)

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
    : server_identifier_(SERVER_IDENTIFIER),
      client_identifier_(CLIENT_IDENTIFIER),
      servers_(),
      clients_(),
      kq_(),
      event_ts_sec_() {}

Multiplexer::~Multiplexer() {}

int Multiplexer::Init(const Configuration& configuration) {
  this->servers_.clear();

  if (InitConfiguration(configuration) == ERROR || InitServer() == ERROR ||
      InitKqueue() == ERROR)
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
  }

  return OK;
}

int Multiplexer::InitKqueue() {
  if ((this->kq_ = kqueue()) == -1) {
    std::cerr << KQUEUE_ERROR_MASSAGE << std::endl;

    return ERROR;
  }

  return OK;
}

int Multiplexer::Multiplexing() {
  // read event 추가
  for (std::vector<Server>::const_iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (RegistKevent(it->fd(), EVFILT_READ, EV_ADD, 0, 0,
                     (void*)&server_identifier_) == ERROR)
      return ERROR;
  }

  return StartServer();
}

int Multiplexer::StartServer() {
  struct kevent events[KEVENT_SIZE];

  while (1) {
    int nev;

    PollingEvent(events, nev);
    HandleEvents(nev, events);
  }
}

int Multiplexer::PollingEvent(struct kevent events[], int& nev) {
  nev = kevent(this->kq_, NULL, 0, events, KEVENT_SIZE, NULL);
  if (nev == -1) {
    std::cerr << KEVENT_ERROR_MASSAGE << std::endl;

    return ERROR;
  }

  return OK;
}

void Multiplexer::HandleEvents(int nev, struct kevent events[]) {
  for (int i = 0; i < nev; ++i) {
    if (events[i].filter == EVFILT_READ) {
      HandleReadEvent(events[i]);
    } else if (events[i].filter == EVFILT_WRITE) {
      HandleWriteEvent(events[i]);
    } else if (events[i].filter == EVFILT_PROC) {
    } else {
      std::cerr << UNDEFINE_FILTER_ERROR_MASSAGE << std::endl;
    }
  }
}

void Multiplexer::HandleReadEvent(struct kevent event) {
  if (*static_cast<int*>(event.udata) == server_identifier_) {
    AcceptWithClient(event.ident);
  } else if (*static_cast<int*>(event.udata) == client_identifier_) {
    Client& client = this->clients_[event.ident];
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(event.ident, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read == -1) {
      close(event.ident);
      this->clients_.erase(event.ident);

      RegistKevent(event.ident, EVFILT_READ, EV_DELETE, 0, 0,
                   (void*)&client_identifier_);
      std::cerr << RECV_ERROR_MASSAGE << std::endl;

      return;
    }
    buffer[bytes_read] = 0;

    client.set_request_str(client.request_str() + buffer);

    if (size_t header_end =
            client.request_str().find("\r\n\r\n") != std::string::npos) {
      ssize_t offset = 0;

      if (client.request_instance().ParseRequestHeader(
              client.request_str().c_str(), client.request_str().length(),
              offset) == ERROR) {
        std::cerr << REQUEST_HEADER_PARSE_ERROR_MASSAGE << std::endl;

        close(event.ident);
        this->clients_.erase(event.ident);

        return;
      }

      if (IsReadyToSend(event.ident, header_end)) {
        RegistKevent(event.ident, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0,
                     (void*)&client_identifier_);
      }
    }
  } else {
    std::cerr << NMANAGE_CLIENT_ERROR_MASSAGE << std::endl;
  }
}

void Multiplexer::HandleWriteEvent(struct kevent event) {
  const Client& client = this->clients_[event.ident];
  Server sk = this->clients_[event.ident].server();

  Request request = this->clients_[event.ident].request();
  request.uri_.ReconstructTargetUri(request.http_host_);

  ServerConfiguration sc = sk.ConfByHost(request.http_host_);

  std::clog << std::endl << " [ Request Start ] " << std::endl;
  std::clog << client.request_str() << std::endl;
  std::clog << std::endl << " [ Request End ] " << std::endl;

  Response response(request, sc);
  response.HttpTransaction();

  std::clog << std::endl << " [ Response Start ] " << std::endl;
  std::clog << response.response_message_ << std::endl;
  std::clog << std::endl << " [ Response End ] " << std::endl;

  send(event.ident, response.response_message_.c_str(),
       response.response_message_.size(), 0);
  std::clog << response.response_message_ << std::endl;
}

void Multiplexer::ReadClientMessage(int client_fd) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

  if (bytes_read == -1) {
    std::cerr << RECV_ERROR_MASSAGE << std::endl;

    close(client_fd);
    this->clients_.erase(client_fd);

    RegistKevent(client_fd, EVFILT_READ, EV_DELETE, 0, 0,
                 (void*)&client_identifier_);

    return;
  }
  buffer[bytes_read] = 0;

  this->clients_[client_fd].set_request_str(
      this->clients_[client_fd].request_str() + buffer);

  if (size_t header_end = this->clients_[client_fd].request_str().find(
                              "\r\n\r\n") != std::string::npos) {
    ssize_t offset = 0;

    if (this->clients_[client_fd].request_instance().ParseRequestHeader(
            this->clients_[client_fd].request_str().c_str(),
            this->clients_[client_fd].request_str().length(),
            offset) == ERROR) {
      std::cerr << REQUEST_HEADER_PARSE_ERROR_MASSAGE << std::endl;

      close(client_fd);
      this->clients_.erase(client_fd);

      return;
    }

    if (IsReadyToSend(client_fd, header_end)) {
      RegistKevent(client_fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0,
                   (void*)&client_identifier_);
    }
  }
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

  // server 찾기
  Server server;

  for (std::vector<Server>::const_iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->fd() == server_fd) {
      this->clients_[client_fd] = Client(*it, client_fd);
      break;
    }
  }

  // client socket fd 관리 추가
  // kqueue에 client socket 등록
  if (RegistKevent(client_fd, EVFILT_READ, EV_ADD, 0, 0,
                   (void*)&client_identifier_) == ERROR)
    return ERROR;

  return OK;
}

int Multiplexer::RegistKevent(int ident, int16_t filter, uint64_t flags,
                              uint32_t fflags, int64_t data, void* udata) {
  struct kevent event;

  struct timespec ts_;
  ts_.tv_sec = EVENT_TIMEOUT_SEC;
  ts_.tv_nsec = 0;

  EV_SET(&event, ident, filter, flags, fflags, data, udata);
  if (kevent(this->kq_, &event, 1, NULL, 0, &ts_) == -1) {
    std::cerr << KEVENT_ERROR_MASSAGE << std::endl;

    return ERROR;
  }

  return OK;
}

bool Multiplexer::IsReadyToSend(int client_fd, size_t header_offset) {
  if (this->clients_[client_fd].request().http_content_length_ >
      0) {  // -1로 교체 예정
    ssize_t body_length =
        this->clients_[client_fd].request_str().length() - (header_offset + 4);

    if (body_length <
        this->clients_[client_fd].request().http_content_length_) {
      return false;
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

Server& Multiplexer::ServerInstanceByPort(int port) {
  for (std::vector<Server>::iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->port() == port) {
      return *it;
    }
  }

  return *this->servers_.end();
}

Client& Multiplexer::ClientInstanceByFd(int fd) {
  for (std::map<int, Client>::iterator it = this->clients_.begin();
       it != this->clients_.end(); it++) {
    if (it->second.fd() == fd) {
      return it->second;
    }
  }

  return this->clients_.end()->second;
}
