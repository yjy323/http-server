#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>  // for `struct sockaddr_in`

#include <vector>

#include "configuration.hpp"

class Server {
 public:
  Server();
  Server(const Server& ref);

  virtual ~Server();

  Server& operator=(const Server& ref);

  void AddConf(const ServerConfiguration& conf);
  void InitAddr();

  // for socket
  int Open();
  int Bind();
  int Listen(int backlog);
  int Close();
  int SetReusable();

  std::vector<ServerConfiguration> conf() const;
  int fd() const;
  int port() const;
  struct sockaddr_in addr() const;

 private:
  struct sockaddr_in AddrOfConf();

  std::vector<ServerConfiguration> conf_;
  int fd_;
  int port_;
  struct sockaddr_in addr_;
};

#endif
