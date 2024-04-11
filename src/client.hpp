#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "request.hpp"
#include "server.hpp"

class Client {
 public:
  Client();
  Client(const Server& server, int fd);
  Client(const Client& ref);
  virtual ~Client();

  Client& operator=(const Client& ref);

  const int& fd() const;
  const Server& server() const;
  const std::string& request_str() const;
  const std::string& response() const;
  const Request& request() const;
  Request& request_instance();

  void set_request_str(const std::string& request);
  void set_response(const std::string& response);

 private:
  Server server_;
  int fd_;
  Request request_;
  std::string request_str_;
  std::string response_;
};

#endif
