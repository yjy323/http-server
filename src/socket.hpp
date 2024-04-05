#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <netinet/in.h>  // for `struct sockaddr_in`

#include "configuration.hpp"

class Socket {
 public:
  Socket(const ServerConfiguration& conf);
  Socket(const Socket& ref);

  virtual ~Socket();

  Socket& operator=(const Socket& ref);

  int Bind();
  int Listen(int backlog);
  int Close();
  int SetReusable();

  ServerConfiguration conf() const;
  int fd() const;
  struct sockaddr_in addr() const;

 private:
  Socket();

  struct sockaddr_in AddrOfConf(const ServerConfiguration& conf);

  ServerConfiguration conf_;
  int fd_;
  struct sockaddr_in addr_;
};

#endif
