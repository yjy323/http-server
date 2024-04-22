#include "socket.hpp"

#include <fcntl.h>

Socket::Socket() : fd_(-1), info_(), nonblock_(false) {}

Socket::Socket(const int& fd, const Info& info)
    : fd_(fd), info_(info), nonblock_(false) {}

Socket::Socket(const Info& info)
    : fd_(socket(info.domain, info.type, info.protocol)),
      info_(info),
      nonblock_(false) {}

Socket::Socket(const Socket& ref)
    : fd_(ref.fd()), info_(ref.info()), nonblock_(ref.nonblock()) {}

bool Socket::Info::operator==(const struct s_Info& info) const {
  return (domain == info.domain && protocol == info.protocol &&
          type == info.type);
}

Socket& Socket::operator=(const Socket& ref) {
  if (&ref == this) return *this;

  fd_ = ref.fd();
  info_ = ref.info();
  nonblock_ = ref.nonblock();

  return *this;
}

#include <iostream>
Socket::~Socket() { close(fd_); }

int Socket::Listen(int backlog) { return listen(fd_, backlog); }

int Socket::Bind(const struct sockaddr* addr) {
  return bind(fd_, addr, sizeof(struct sockaddr_in));
}

int Socket::Setsockopt(int level, int option_name, const void* option_value,
                       socklen_t option_len) {
  return setsockopt(fd_, level, option_name, option_value, option_len);
}

int Socket::SetNonBlock() {
  if (nonblock_ == true) return 0;

  int rtn = 0;
  int flag = fcntl(fd_, F_GETFL, 0);
  rtn = fcntl(fd_, F_SETFL, flag | O_NONBLOCK);
  if (rtn != -1) {
    nonblock_ = true;
  }

  return 0;
}

const int& Socket::fd() const { return fd_; }
int& Socket::fd() { return fd_; }
const Socket::Info& Socket::info() const { return info_; }
const bool& Socket::nonblock() const { return nonblock_; }