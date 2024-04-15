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

  int InitConfiguration(const Configuration& configuration);
  int InitServer();
  int InitKqueue();
  int StartServer();
  int PollingEvent(struct kevent events[], int& nev);
  void HandleEvents(int nev, struct kevent events[]);
  void HandleReadEvent(struct kevent event);
  void HandleWriteEvent(struct kevent event);
  void HandleCgiEvent(struct kevent event);
  int ReadClientMessage(int client_fd, std::string& message);
  int AcceptWithClient(int server_fd);
  int CloseWithClient(int client_fd);
  int RegistKevent(int ident, int16_t filter, uint64_t flags, uint32_t fflags,
                   int64_t data, void* udata);
  bool IsReadyToSend(int client_fd, size_t header_offset);
  bool IsExistPort(int port);
  void AddConfInServers(const ServerConfiguration& server_conf);

  Server& ServerInstanceByPort(int port);

  int server_udata_;
  int client_udata_;
  int cgi_udata_;

  std::vector<Server> servers_;
  std::map<int, Client> clients_;
  int kq_;
  long event_ts_sec_;
};

#endif
