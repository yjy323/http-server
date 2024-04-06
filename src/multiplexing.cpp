#include "multiplexing.hpp"

#include <arpa/inet.h>  // for `inet_ntoa`
#include <sys/event.h>
#include <unistd.h>

#include "core.hpp"
#include "file_reader.hpp"

#define IS_REUSABLE true
#define DEFAULT_BACKLOG 128
#define KEVENT_SIZE 10
#define BUFFER_SIZE 1024

// for test
#define TEST_RESPONSE \
  "HTTP/1.1 200 OK\r\nContent-Length: 22\r\n\r\nThis is Test Response."

int Multiplexing(const Configuration& configuration);
int Multiplexing(const std::vector<Socket>& sockets);
int StartServer(const std::set<int>& server_fd_set, int kq);
void HandleEvents(int nev, const std::set<int>& server_fd_set,
                  std::vector<int>& client_fds, struct kevent events[], int kq);
int AcceptWithClient(int kq, int server_fd, std::vector<int>& client_fds);

int Multiplexing(const Configuration& configuration) {
  std::vector<Socket> sockets;

  for (Configuration::const_iterator it = configuration.begin();
       it != configuration.end(); it++) {
    Socket socket = Socket(*it);

    if (IS_REUSABLE) socket.SetReusable();

    if (socket.Bind() != 0) {
      std::cerr << "Error: Bind failed [port: " << socket.conf().port() << "]"
                << std::endl;

      return ERROR;
    } else if (socket.Listen(DEFAULT_BACKLOG) != 0) {
      std::cerr << "Error: Listen failed [port: " << socket.conf().port() << "]"
                << std::endl;

      return ERROR;
    }

    std::cout << "Socket " << socket.fd()
              << " is successful binding and listening." << std::endl;

    sockets.push_back(socket);
  }

  return Multiplexing(sockets);
}

int Multiplexing(const std::vector<Socket>& sockets) {
  std::set<int> server_fd_set;
  int kq;
  struct kevent event;

  // kqueue 생성
  if ((kq = kqueue()) == -1) {
    std::cerr << "Error: kqueue creation failed" << std::endl;

    return ERROR;
  }

  for (std::vector<Socket>::const_iterator it = sockets.begin();
       it != sockets.end(); it++) {
    server_fd_set.insert(it->fd());
  }

  // read event 추가
  for (std::set<int>::const_iterator it = server_fd_set.begin();
       it != server_fd_set.end(); it++) {
    int fd = *it;

    EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(kq, &event, 1, NULL, 0, NULL) == ERROR) {
      return ERROR;
    }
  }

  for (std::vector<Socket>::const_iterator it = sockets.begin();
       it != sockets.end(); it++) {
    server_fd_set.insert(it->fd());
  }

  return StartServer(server_fd_set, kq);
}

int StartServer(const std::set<int>& server_fd_set, int kq) {
  struct kevent events[KEVENT_SIZE];
  std::vector<int> client_fds;

  while (1) {
    // event 대기
    int nev = kevent(kq, NULL, 0, events, KEVENT_SIZE, NULL);
    if (nev == -1) {
      std::cerr << "kevent error" << std::endl;
      return ERROR;
    }

    HandleEvents(nev, server_fd_set, client_fds, events, kq);
  }
}

void HandleEvents(int nev, const std::set<int>& server_fd_set,
                  std::vector<int>& client_fds, struct kevent events[],
                  int kq) {
  for (int i = 0; i < nev; ++i) {
    // 연결 요청일 경우
    if (server_fd_set.find(events[i].ident) != server_fd_set.end()) {
      AcceptWithClient(kq, events[i].ident, client_fds);
    } else {  // 데이터 송신일 경우
      // client socket 내용 read
      char buffer[BUFFER_SIZE];
      ssize_t bytes_read = recv(events[i].ident, buffer, sizeof(buffer), 0);
      if (bytes_read <= 0) {
        if (bytes_read == 0) {
          std::cout << "Client " << events[i].ident << " disconnected "
                    << std::endl;

        } else {
          std::cerr << "Error reading from client" << std::endl;
        }

        // client socket close & client_fds에서 제거
        close(events[i].ident);
        client_fds.erase(
            std::remove(client_fds.begin(), client_fds.end(), events[i].ident),
            client_fds.end());

        // kqueue에서 client fd 등록해제
        struct kevent event;
        EV_SET(&event, events[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(kq, &event, 1, NULL, 0, NULL);
      } else {
        std::string response = TEST_RESPONSE;

        std::cout << " [Request] " << std::endl;
        std::cout << buffer << std::endl;

        // 적절한 response 생성 필요 // todo

        send(events[i].ident, response.c_str(), response.size(), 0);
      }
    }
  }
}

int AcceptWithClient(int kq, int server_fd, std::vector<int>& client_fds) {
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int client_fd =
      accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
  if (client_fd == -1) {
    std::cerr << "Accept failed" << std::endl;
    return ERROR;
  }

  // client socket fd 관리 추가
  client_fds.push_back(client_fd);

  // kqueue에 client socket 등록
  struct kevent event;
  EV_SET(&event, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(kq, &event, 1, NULL, 0, NULL) == -1) {
    std::cerr << "Failed to register client socket with kqueue" << std::endl;
    close(client_fd);
    return ERROR;
  }

  return OK;
}
