#ifndef MULTIPLEXER_HPP
#define MULTIPLEXER_HPP

#include <map>
#include <set>
#include <vector>

#include "client.hpp"
#include "configuration.hpp"
#include "request.hpp"
#include "response.hpp"
#include "socket.hpp"

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

  std::vector<Socket> servers_;
  std::map<int, Client> clients_;
  std::set<int> server_fds_;
  int kq_;
};

#endif
