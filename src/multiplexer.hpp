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

  int Init(const Configuration& config);
  int StartServers();
  int OpenServers();

 protected:
 private:
  typedef std::map<int, Server*> Servers;
  typedef std::map<int, Client*> Clients;

  Multiplexer(const Multiplexer& ref);

  Multiplexer& operator=(const Multiplexer& ref);

  int InitServers(const Configuration& config);
  int InitServer(const Server::Configurations& config);
  int StartServer(Server& server);

  void HandleEvents(int nev);
  void HandleErrorEvent(struct kevent& event);
  void HandleReadEvent(struct kevent& event);
  void HandleWriteEvent(struct kevent& event);
  void HandleCgiEvent(struct kevent& event);
  void HandleTimeoutEvent(struct kevent& event);

  int AcceptWithClient(int server_fd);
  void DisconnetClient(Client& client);

  Servers servers_;
  Clients clients_;
  EventHandler eh_;
};

#endif
