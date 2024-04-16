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
#define SERVER_UDATA (1 << 0)
#define CLIENT_UDATA (1 << 1)
#define CGI_UDATA (1 << 1)

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

    std::clog << "server[ fd: " << it->fd()
              << " ] open 완료 (open, bind, listen)" << std::endl;
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
                     static_cast<void*>(&this->server_udata_)) == ERROR)
      return ERROR;

    std::clog << "server[ fd: " << it->fd() << " ] listen event 등록 완료"
              << std::endl;
  }

  return StartServer();
}

int Multiplexer::StartServer() {
  struct kevent events[KEVENT_SIZE];

  while (1) {
    int nev;

    std::clog << "event 대기상태 진입" << std::endl;
    if (PollingEvent(events, nev) == ERROR) continue;
    if (events->flags & EV_ERROR) CloseWithClient(events[nev].ident);
    HandleEvents(nev, events);
  }
}

int Multiplexer::PollingEvent(struct kevent events[], int& nev) {
  struct timespec timeout;
  timeout.tv_sec = EVENT_TIMEOUT_SEC;
  timeout.tv_nsec = 0;

  nev = kevent(this->kq_, NULL, 0, events, KEVENT_SIZE, &timeout);
  if (nev == -1) {
    std::cerr << KEVENT_ERROR_MASSAGE << std::endl;

    return ERROR;
  } else if (nev == 0) {
    std::clog << "Timeout 발생!" << std::endl;

    return ERROR;
  }

  std::clog << nev << "개의 event catch!" << std::endl;

  return OK;
}

void Multiplexer::HandleEvents(int nev, struct kevent events[]) {
  for (int i = 0; i < nev; ++i) {
    if (events[i].filter == EVFILT_READ) {
      std::clog << "[event type] READ" << std::endl;
      HandleReadEvent(events[i]);
    } else if (events[i].filter == EVFILT_WRITE) {
      std::clog << "[event type] WRITE" << std::endl;
      HandleWriteEvent(events[i]);
      // CloseWithClient(events[i].ident);
    } else if (events[i].filter == EVFILT_PROC) {
      std::clog << "[event type] PROC" << std::endl;
      // HandleCgiEvent(events[i]);
    } else if (events[i].filter == EVFILT_TIMER) {
      std::clog << "[event type] TIMER" << std::endl;
      RegistKevent(events[i].ident, EVFILT_READ, EV_DELETE, 0, 0,
                   static_cast<void*>(&this->client_udata_));
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

    if (size_t header_end =
            client.request_str().find(CRLF) != std::string::npos) {
      ssize_t offset = 0;

      if (client.request_instance().ParseRequestHeader(
              client.request_str().c_str(), client.request_str().length(),
              offset) == ERROR) {
        std::cerr << REQUEST_HEADER_PARSE_ERROR_MASSAGE << std::endl;

        std::clog << "[ Parse Error Request ]" << std::endl
                  << client.request_str() << std::endl;

        CloseWithClient(client.fd());

        return;
      }

      if (IsReadyToSend(event.ident, header_end)) {
        std::clog << "server[ " << client.server().fd() << " ]: client[ "
                  << client.fd() << " ]의 Full 메세지 수신 완료" << std::endl;
        std::clog << std::endl << " [ Full Request Start ] " << std::endl;
        std::clog << client.request_str() << std::endl;
        std::clog << std::endl << " [ Full Request End ] " << std::endl;

        client.MakeResponse();
        RegistKevent(event.ident, EVFILT_WRITE, EV_ADD | EV_ONESHOT,
                     NOTE_SECONDS, 0, static_cast<void*>(&this->client_udata_));
      }
    }
  } else {
    std::cerr << NMANAGE_CLIENT_ERROR_MASSAGE << std::endl;
  }
}

void Multiplexer::HandleWriteEvent(struct kevent event) {
  Client& client = this->clients_[event.ident];

  std::clog << std::endl << " [ Response Start ] " << std::endl;
  std::clog << client.response().response_message_ << std::endl;
  std::clog << std::endl << " [ Response End ] " << std::endl;

  std::string response_message = client.response().response_message_;
  send(client.fd(), response_message.c_str(), response_message.length(), 0);
}

void Multiplexer::HandleCgiEvent(struct kevent event) {
  (void)event;
  // // cgi 처리
  // int pid = -1 /* cgi 처리 */;
  // RegistKevent(pid, EVFILT_PROC, EV_ADD | EV_ONESHOT, 0, 0,
  //              static_cast<void*>(&this->cgi_udata_));
}

int Multiplexer::ReadClientMessage(int client_fd, std::string& message) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

  if (bytes_read == -1) {
    CloseWithClient(client_fd);

    RegistKevent(client_fd, EVFILT_READ, EV_DELETE, 0, 0,
                 static_cast<void*>(&this->client_udata_));
    RegistKevent(client_fd, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
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
  if ((RegistKevent(client_fd, EVFILT_READ, EV_ADD, 0, 0,
                    static_cast<void*>(&this->client_udata_)) == ERROR) ||
      (RegistKevent(client_fd, EVFILT_TIMER, EV_ONESHOT | NOTE_SECONDS, 0, 5000,
                    static_cast<void*>(&this->client_udata_)) == ERROR))
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
