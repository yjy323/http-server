#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

class Socket {
 public:
  typedef struct s_Info {
    int domain;
    int type;
    int protocol;

    bool operator==(const struct s_Info& info) const;
  } Info;

  Socket(const Info& info);
  Socket(const Socket& ref);

  ~Socket();

  Socket& operator=(const Socket& ref);

  int Listen(int backlog);
  int Bind(const struct sockaddr* addr);
  int Setsockopt(int level, int option_name, const void* option_value,
                 socklen_t option_len);
  int SetNonBlock();

  const int& fd() const;
  int& fd();
  const Info& info() const;
  const bool& nonblock() const;

 protected:
  Socket();
  Socket(const int& fd, const Info& info);

 private:
  int fd_;
  Info info_;
  bool nonblock_;
};

#endif