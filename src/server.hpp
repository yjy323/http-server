#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>  // for `struct sockaddr_in`

#include "configuration.hpp"

class Server {
 public:
  Server(const ServerConfiguration& conf);
  Server(const Server& ref);

  virtual ~Server();

  Server& operator=(const Server& ref);

  int Bind();
  int Listen(int backlog);
  int Close();
  int SetReusable();

  ServerConfiguration conf() const;
  int fd() const;
  int port() const;
  struct sockaddr_in addr() const;

 private:
  Server();

  struct sockaddr_in AddrOfConf();

  ServerConfiguration conf_;
  int fd_;
  int port_;
  struct sockaddr_in addr_;
};

#endif
