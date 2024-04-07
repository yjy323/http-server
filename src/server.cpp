#include "server.hpp"

#include <unistd.h>

#include "core.hpp"
#include "utils.hpp"

#define NEW_SOCKET_FAIL_MESSGAE std::string("socket() failed.")
#define BIND_FAIL_MASSAGE std::string("bind() to 0.0.0.0:`port` failed.")
#define LISTEN_FAIL_MASSAGE std::string("listen() to 0.0.0.0:`port` failed.")
#define SETSOCKOPT_FAIL_MASSAGE std::string("setsockopt() failed.")

Server::Server(const ServerConfiguration& conf)
    : conf_(conf),
      fd_(socket(AF_INET, SOCK_STREAM, 0)),
      port_(conf.port()),
      addr_(AddrOfConf()) {
  if (this->fd_ == -1) std::cerr << NEW_SOCKET_FAIL_MESSGAE << std::endl;
}

Server::Server(const Server& ref)
    : conf_(ref.conf()), fd_(ref.fd()), port_(ref.port()), addr_(ref.addr()) {}

Server::~Server() {}

Server& Server::operator=(const Server& ref) {
  if (this == &ref) return *this;

  this->conf_ = ref.conf();
  this->fd_ = ref.fd();
  this->addr_ = ref.addr();

  return *this;
}

/* method */

int Server::Bind() {
  int sockfd = this->fd_;
  const sockaddr* addr = (struct sockaddr*)(&this->addr_);
  socklen_t addrlen = sizeof(this->addr_);

  if (bind(sockfd, addr, addrlen) == -1) {
    std::stringstream ss;
    ss << this->port_;

    std::cerr << BIND_FAIL_MASSAGE.replace(BIND_FAIL_MASSAGE.find("`port`"),
                                           std::string("`port`").length(),
                                           ss.str())
              << std::endl;

    return ERROR;
  }

  return OK;
}

int Server::Listen(int backlog) {
  int sockfd = this->fd_;

  if (listen(sockfd, backlog) == -1) {
    std::stringstream ss;
    ss << this->port_;

    std::cerr << LISTEN_FAIL_MASSAGE.replace(LISTEN_FAIL_MASSAGE.find("`port`"),
                                             std::string("`port`").length(),
                                             ss.str())
              << std::endl;

    return ERROR;
  }

  return OK;
}

int Server::Close() { return close(this->fd_); }

int Server::SetReusable() {
  int isreusable = 1;

  if (setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &isreusable,
                 sizeof(isreusable)) == -1) {
    std::cerr << SETSOCKOPT_FAIL_MASSAGE << std::endl;

    return ERROR;
  }
  return OK;
}

struct sockaddr_in Server::AddrOfConf() {
  struct sockaddr_in addr;

  Memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(this->port_);

  return addr;
}

/* getter */

ServerConfiguration Server::conf() const { return this->conf_; }

int Server::fd() const { return this->fd_; }

int Server::port() const { return this->port_; }

struct sockaddr_in Server::addr() const { return this->addr_; }
