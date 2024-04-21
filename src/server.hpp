#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>  // for `struct sockaddr_in`

#include <vector>

#include "configuration.hpp"
#include "socket.hpp"

class Server : public Socket {
 public:
  typedef ServerConfiguration Configuration;
  typedef std::vector<Configuration> Configurations;

  Server(const Info& info, const Configurations& conf_);
  Server(const Server& ref);

  virtual ~Server();

  Server& operator=(const Server& ref);
  int Bind();
  int Accept();
  Configuration ConfByHost(const std::string& host) const;

  const Configurations& config() const;
  const int& port() const;
  const bool& reusable() const;

 protected:
  Server();

 private:
  Configurations config_;
  int port_;
  bool reusable_;
};

#endif
