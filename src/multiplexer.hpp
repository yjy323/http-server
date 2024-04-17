#ifndef MULTIPLEXER_HPP
#define MULTIPLEXER_HPP

#include <map>
#include <vector>

#include "client.hpp"
#include "configuration.hpp"
#include "event_handler.hpp"
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

  int StartServer();
  void HandleEvents(int nev);
  void HandleReadEvent(struct kevent event);
  void HandleWriteEvent(struct kevent event);
  void HandleCgiEvent(struct kevent event);
  int ReadClientMessage(int client_fd, std::string& message);
  int AcceptWithClient(int server_fd);
  int CloseWithClient(int client_fd);

  bool IsReadFullBody(int client_fd, size_t header_offset);
  bool IsExistPort(int port);
  void AddConfInServers(const ServerConfiguration& server_conf);

  Server& ServerInstanceByPort(int port);

  int server_udata_;
  int client_udata_;
  int cgi_udata_;

  std::vector<Server> servers_;
  std::map<int, Client> clients_;
  // int kq_;
  EventHandler eh_;
};

#endif
