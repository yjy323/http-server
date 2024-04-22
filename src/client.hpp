#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "server.hpp"
#include "socket.hpp"
#include "transaction.hpp"

class Client : public Socket {
 public:
  Client(const Server& server, int fd);
  virtual ~Client();

  Client& operator=(const Client& ref);

  void ResetClientInfo();

  ssize_t ReceiveRequest();
  int ParseRequestHeader();
  int ParseRequestBody();
  void CreateResponseMessage();
  void CreateResponseMessageByCgi();
  int SendResponseMessage();

  bool IsReceiveRequestHeaderComplete();
  bool IsReceiveRequestBodyComplete();

  const Server::Configurations& server_configs() const;
  const std::string& request() const;
  const std::string& response() const;
  const Transaction& transaction() const;
  Transaction& transaction();

  void set_request(const std::string& request);
  void set_response(const std::string& response);

 protected:
  Client();
  Client(const Client& ref);

 private:
  int ReadCgi(std::string& cgi_res);

  bool IsRequestBodyAllowedMethod();

  const Server::Configuration& conf_by_host(const std::string& host);
  const std::string request_body();

  Server::Configurations server_configs_;
  Transaction transaction_;
  std::string request_;
  std::string response_;

  bool request_body_parsed_;
};

#endif
