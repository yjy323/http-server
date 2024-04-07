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
#define RECV_ERROR_MASSAGE "recv() from client failed."
#define ACCEPT_ERROR_MASSAGE "accept() from client failed."

// for test
#define TEST_RESPONSE \
  "HTTP/1.1 200 OK\r\nContent-Length: 22\r\n\r\nThis is Test Response."

Multiplexer::Multiplexer() : servers_(), server_fds_(), kq_() {}

Multiplexer::~Multiplexer() {}

int Multiplexer::Init(const Configuration& configuration) {
  this->servers_.clear();
  this->server_fds_.clear();

  for (Configuration::const_iterator it = configuration.begin();
       it != configuration.end(); it++) {
    Server server = Server(*it);

    if (server.fd() == -1) return ERROR;
    if (IS_REUSABLE) server.SetReusable();

    if (server.Bind() == ERROR || server.Listen(DEFAULT_BACKLOG) == ERROR) {
      return ERROR;
    }

    this->servers_.push_back(server);
    this->server_fds_.insert(server.fd());
  }

  if ((this->kq_ = kqueue()) == -1) {
    std::cerr << KQUEUE_ERROR_MASSAGE << std::endl;

    return ERROR;
  }

  return OK;
}

int Multiplexer::Multiplexing() {
  // read event 추가
  for (std::set<int>::const_iterator it = this->server_fds_.begin();
       it != this->server_fds_.end(); it++) {
    int fd = *it;

    struct kevent event;
    EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(this->kq_, &event, 1, NULL, 0, NULL) == ERROR) {
      return ERROR;
    }
  }

  for (std::vector<Server>::const_iterator it = this->servers_.begin();
       it != this->servers_.end(); it++) {
    this->server_fds_.insert(it->fd());
  }

  return StartServer();
}

int Multiplexer::StartServer() {
  struct kevent events[KEVENT_SIZE];

  while (1) {
    // event 대기
    int nev = kevent(this->kq_, NULL, 0, events, KEVENT_SIZE, NULL);
    if (nev == -1) {
      std::cerr << KQUEUE_ERROR_MASSAGE << std::endl;
      return ERROR;
    }

    HandleEvents(nev, events);
  }
}

void Multiplexer::HandleEvents(int nev, struct kevent events[]) {
  for (int i = 0; i < nev; ++i) {
    // 연결 요청일 경우
    if (this->server_fds_.find(events[i].ident) != this->server_fds_.end()) {
      AcceptWithClient(events[i].ident);
    } else {  // 데이터 송신일 경우
      // client socket 내용 read
      char buffer[BUFFER_SIZE];
      ssize_t bytes_read = recv(events[i].ident, buffer, sizeof(buffer) - 1, 0);
      if (bytes_read <= 0) {
        if (bytes_read == 0) {
          std::cout << "Client " << events[i].ident << " disconnected "
                    << std::endl;
        } else {
          std::cerr << RECV_ERROR_MASSAGE << std::endl;
        }

        // Close client socket and remove it from the vector
        close(events[i].ident);
        this->clients_.erase(events[i].ident);

        // Unregister client socket from kqueue
        struct kevent event;
        EV_SET(&event, events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(this->kq_, &event, 1, NULL, 0, NULL);
      } else {
        buffer[bytes_read] = 0;
        std::string response = TEST_RESPONSE;

        std::cout << " [Request] " << std::endl;
        std::cout << buffer << std::endl;
        send(events[i].ident, response.c_str(), response.size(), 0);
      }
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

  // client socket fd 관리 추가
  this->clients_[client_fd] = Client(client_fd);

  // kqueue에 client socket 등록
  struct kevent event;
  EV_SET(&event, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(this->kq_, &event, 1, NULL, 0, NULL) == -1) {
    std::cerr << "Failed to register client socket with kqueue" << std::endl;
    close(client_fd);
    return ERROR;
  }

  return OK;
}
