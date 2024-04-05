#include "socket.hpp"

#include <unistd.h>

#include "utils.hpp"

Socket::Socket(const ServerConfiguration& conf)
    : conf_(conf),
      fd_(socket(AF_INET, SOCK_STREAM, 0)),
      addr_(AddrOfConf(conf)) {}

Socket::Socket(const Socket& ref)
    : conf_(ref.conf()), fd_(ref.fd()), addr_(ref.addr()) {}

Socket::~Socket() {}

Socket& Socket::operator=(const Socket& ref) {
  if (this == &ref) return *this;

  this->conf_ = ref.conf();
  this->fd_ = ref.fd();
  this->addr_ = ref.addr();

  return *this;
}

/* method */

int Socket::Bind() {
  int sockfd = this->fd_;
  const sockaddr* addr = (struct sockaddr*)(&this->addr_);
  socklen_t addrlen = sizeof(this->addr_);

  return bind(sockfd, addr, addrlen);
}

int Socket::Listen(int backlog) {
  int sockfd = this->fd_;

  return listen(sockfd, backlog);
}

int Socket::Close() { return close(this->fd_); }

int Socket::SetReusable() {
  int isreusable = 1;

  return setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &isreusable,
                    sizeof(isreusable));
}

struct sockaddr_in Socket::AddrOfConf(const ServerConfiguration& conf) {
  struct sockaddr_in addr;

  Memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(conf.port());

  return addr;
}

/* getter */

ServerConfiguration Socket::conf() const { return this->conf_; }

int Socket::fd() const { return this->fd_; }

struct sockaddr_in Socket::addr() const { return this->addr_; }
