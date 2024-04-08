#ifndef MULTIPLEXER_HPP
#define MULTIPLEXER_HPP

#include <map>
#include <set>
#include <vector>

#include "client.hpp"
#include "configuration.hpp"
#include "server.hpp"

class Multiplexer {
 public:
  Multiplexer();
  virtual ~Multiplexer();

  int Init(const Configuration& configuration);
  int Multiplexing();

 private:
  Multiplexer(const Multiplexer& ref);

  Multiplexer& operator=(const Multiplexer& ref);

  int StartServer();
  void HandleEvents(int nev, struct kevent events[]);
  int AcceptWithClient(int server_fd);
  bool IsExistPort(int port);
  void AddConfInServers(const ServerConfiguration& server_conf);

  Server& ServerInstanceByPort(int port);

  std::vector<Server> servers_;
  std::map<int, Client> clients_;
  std::set<int> server_fds_;
  int kq_;
};

#endif
