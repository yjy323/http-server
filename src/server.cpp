#include "server.hpp"

#include <unistd.h>

#include "core.hpp"
#include "utils.hpp"

Server::Server() : Socket(), config_(), port_(-1), reusable_(false) {}

Server::Server(const Info& info, const Server::Configurations& conf)
    : Socket(info), config_(conf), port_(conf[0].port()), reusable_(false) {}

Server::Server(const Server& ref)
    : Socket(ref),
      config_(ref.config()),
      port_(ref.port()),
      reusable_(ref.reusable()) {}

Server::~Server() {}

Server& Server::operator=(const Server& ref) {
  if (this == &ref) return *this;

  Socket::operator=(ref);
  this->config_ = ref.config();
  this->reusable_ = ref.reusable();

  return *this;
}

/* method */

int Server::Bind() {
  struct sockaddr_in addr;

  Memset(&(addr), 0, sizeof(addr));

  addr.sin_family = this->info().domain;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port_);

  return Socket::Bind((struct sockaddr*)&addr);
}

int Server::Accept() {
  struct sockaddr client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  return accept(Socket::fd(), &client_addr, &client_addr_len);
}

ServerConfiguration Server::ConfByHost(const std::string& host) const {
  for (Server::Configurations::const_iterator it = this->config_.begin();
       it != this->config_.end(); it++) {
    for (std::set<std::string>::iterator it_server_host =
             it->server_names().begin();
         it_server_host != it->server_names().end(); it_server_host++) {
      if (*it_server_host == host) {
        return *it;
      }
    }
  }

  return this->config_[0];
}

/* getter */

const Server::Configurations& Server::config() const { return this->config_; }
const int& Server::port() const { return this->port_; }
const bool& Server::reusable() const { return this->reusable_; }
