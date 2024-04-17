#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "server.hpp"
#include "transaction.hpp"

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
  const std::string& response_str() const;

  const Transaction& transaction() const;
  Transaction& transaction_instance();

  void set_request_str(const std::string& request);
  void set_response_str(const std::string& response);

 private:
  Server server_;
  int fd_;
  Transaction transaction_;
  std::string request_str_;
  std::string response_str_;
};

#endif
