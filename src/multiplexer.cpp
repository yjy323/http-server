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

#define KQUEUE_ERROR_MASSAGE "kqueue() failed."
#define KEVENT_ERROR_MASSAGE "kevent() failed."
#define RECV_ERROR_MASSAGE "recv() from client failed."
#define ACCEPT_ERROR_MASSAGE "accept() from client failed."
#define UNDEFINE_FILTER_ERROR_MASSAGE "catch undefine filter."
#define NMANAGE_CLIENT_ERROR_MASSAGE \
  "A message request to a client that is not being managed."
#define REQUEST_HEADER_PARSE_ERROR_MASSAGE \
  "An error occurred while parsing header."

// for test
#define TEST_RESPONSE \
  "HTTP/1.1 200 OK\r\nContent-Length: 22\r\n\r\nThis is Test Response."

Multiplexer::Multiplexer() : servers_(), kq_() {}

Multiplexer::~Multiplexer() {}

int Multiplexer::Init(const Configuration& configuration) {
  this->servers_.clear();

  for (Configuration::const_iterator it = configuration.begin();
       it != configuration.end(); it++) {
    AddConfInServers(*it);
  }

  for (std::vector<Server>::iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->Open() == ERROR || (IS_REUSABLE && it->SetReusable() == ERROR) ||
        it->Bind() == ERROR || it->Listen(DEFAULT_BACKLOG) == ERROR)
      return ERROR;
  }

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
    int fd = it->fd();

    if (RegistKevent(fd, EVFILT_READ, EV_ADD, 0, 0, NULL) == ERROR)
      return ERROR;
  }

  return StartServer();
}

int Multiplexer::StartServer() {
  struct kevent events[KEVENT_SIZE];

  while (1) {
    // event 대기
    int nev = kevent(this->kq_, NULL, 0, events, KEVENT_SIZE, NULL);
    if (nev == -1) {
      std::cerr << KEVENT_ERROR_MASSAGE << std::endl;
      return ERROR;
    }

    HandleEvents(nev, events);
  }
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
  if (IsExistServerFd(event.ident)) {
    AcceptWithClient(event.ident);
  } else {  // 데이터 송신일 경우
    if (this->clients_.find(event.ident) == this->clients_.end()) {
      std::cerr << NMANAGE_CLIENT_ERROR_MASSAGE << std::endl;
    }

    // client socket 내용 read
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(event.ident, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read == -1) {
      // Close client socket and remove it from the vector
      close(event.ident);
      this->clients_.erase(event.ident);

      // Unregister client socket from kqueue
      RegistKevent(event.ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
      std::cerr << RECV_ERROR_MASSAGE << std::endl;

      return;
    }
    buffer[bytes_read] = 0;

    this->clients_[event.ident].set_request_str(
        this->clients_[event.ident].request_str() + buffer);

    std::clog << " [ Recive Request Start ] " << std::endl;
    std::clog << buffer << std::endl;
    std::clog << " [ Recive Request End ] " << std::endl;
  }
  if (size_t header_end = this->clients_[event.ident].request_str().find(
                              "\r\n\r\n") != std::string::npos) {
    ssize_t offset = 0;

    if (this->clients_[event.ident].request_instance().ParseRequestHeader(
            this->clients_[event.ident].request_str().c_str(),
            this->clients_[event.ident].request_str().length(),
            offset) == ERROR) {
      std::cerr << REQUEST_HEADER_PARSE_ERROR_MASSAGE << std::endl;
      close(event.ident);
      this->clients_.erase(event.ident);

      return;
    }

    if (this->clients_[event.ident].request().http_content_length_ ==
        0) {  // -1로 교체 예정
      RegistKevent(event.ident, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
    } else {
      ssize_t body_length =
          this->clients_[event.ident].request_str().length() - (header_end + 4);

      if (body_length >=
          this->clients_[event.ident].request().http_content_length_) {
        RegistKevent(event.ident, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0,
                     NULL);
      }
    }
  }
}

void Multiplexer::HandleWriteEvent(struct kevent event) {
  Client client = this->clients_[event.ident];
  Request request;
  Server sk = this->clients_[event.ident].server();

  std::clog << " [Request] " << std::endl;
  ssize_t offset = 0;
  request.ParseRequestHeader(client.request_str().c_str(),
                             client.request_str().length(), offset);
  request.uri_.ReconstructTargetUri(request.http_host_);

  ServerConfiguration sc = sk.ConfByHost(request.http_host_);

  std::clog << " [ Request Start ] " << std::endl;
  std::clog << client.request_str() << std::endl;
  std::clog << " [ Request End ] " << std::endl;

  Response response(request, sc);
  response.HttpTransaction();

  std::clog << " [ Response Start ] " << std::endl;
  std::clog << response.response_message_ << std::endl;
  std::clog << " [ Response End ] " << std::endl;

  send(event.ident, response.response_message_.c_str(),
       response.response_message_.size(), 0);
  std::clog << response.response_message_ << std::endl;
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
  if (RegistKevent(client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL) == ERROR)
    return ERROR;
  return OK;
}

int Multiplexer::RegistKevent(int ident, int16_t filter, uint64_t flags,
                              uint32_t fflags, int64_t data, uint64_t* udata) {
  struct kevent event;

  EV_SET(&event, ident, filter, flags, fflags, data, udata);
  if (kevent(this->kq_, &event, 1, NULL, 0, NULL) == -1) {
    std::cerr << KEVENT_ERROR_MASSAGE << std::endl;

    return ERROR;
  }

  return OK;
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

bool Multiplexer::IsExistServerFd(int fd) {
  for (std::vector<Server>::const_iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    if (it->fd() == fd) return true;
  }

  return false;
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
