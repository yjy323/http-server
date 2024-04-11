#ifndef MULTIPLEXER_HPP
#define MULTIPLEXER_HPP

#include <map>
#include <vector>

#include "client.hpp"
#include "configuration.hpp"
#include "request.hpp"
#include "response.hpp"
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
  void HandleReadEvent(struct kevent event);
  void HandleWriteEvent(struct kevent event);
  int AcceptWithClient(int server_fd);
  int RegistKevent(int ident, int16_t filter, uint64_t flags, uint32_t fflags,
                   int64_t data, uint64_t* udata);
  bool IsExistServerFd(int fd);
  bool IsExistPort(int port);
  void AddConfInServers(const ServerConfiguration& server_conf);

  Server& ServerInstanceByPort(int port);
  Client& ClientInstanceByFd(int fd);

  std::vector<Server> servers_;
  std::map<int, Client> clients_;
  int kq_;
};

#endif
