#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "request.hpp"
#include "response.hpp"
#include "server.hpp"

class Client {
 public:
  Client();
  Client(const Server& server, int fd);
  Client(const Client& ref);
  virtual ~Client();

  Client& operator=(const Client& ref);

  void MakeResponse();

  const int& fd() const;
  const Server& server() const;
  const std::string& request_str() const;
  const std::string& response_str() const;
  const Request& request() const;
  const Response& response() const;
  Request& request_instance();
  Response& response_instance();

  void set_request_str(const std::string& request);
  void set_response_str(const std::string& response);

 private:
  Server server_;
  int fd_;
  Request request_;
  Response response_;
  std::string request_str_;
  std::string response_str_;
};

#endif
