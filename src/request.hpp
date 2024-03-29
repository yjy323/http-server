#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <sys/socket.h>

#include <iostream>
#include <map>
#include <vector>

#include "abnf.hpp"
#include "core.hpp"
#include "url.hpp"
#include "utils.hpp"

#define BUFFER_SIZE 4096

class Request {
 private:
  std::string method_;
  Url url_;
  int major_version_;
  int minor_version_;

  std::multimap<const std::string, std::string> headers_;
  std::string body_;

  int ParseMethod(std::string& method);
  int ParseRequestTarget(std::string& request_target);
  int ParseHttpVersion(std::string& http_version);
  int ParseRequestLine(std::string& request_line);

  int ParseHttpHeader(std::string& header);

 public:
  Request();
  Request(const Request& obj);
  ~Request();
  Request& operator=(const Request& obj);

  int ReceiveRequest(char* buff, ssize_t size);
};

#endif
