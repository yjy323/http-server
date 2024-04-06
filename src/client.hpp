#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
 public:
  Client();
  Client(int fd);
  Client(const Client& ref);
  virtual ~Client();

  Client& operator=(const Client& ref);

  const int& fd() const;
  const std::string& request() const;
  const std::string& response() const;

  void set_request(const std::string& request);
  void set_response(const std::string& response);

 private:
  int fd_;
  std::string request_;
  std::string response_;
};

#endif
