#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

#include "abnf.hpp"
#include "http.hpp"
#include "uri.hpp"
#include "utils.hpp"

class Request {
 public:  // todo private으로 바꿀 지 고려
  std::string method_;
  Uri uri_;

  HeadersIn headers_in_;
  std::string body_;

  int ParseRequestLine(std::string& request_line);
  int ParseFieldValue(std::string& header);

  int ParseRequestHeader(const char* buff, ssize_t size, ssize_t& offset);
  int ParseRequestBody(char* buff, ssize_t size, ssize_t& offset);
  int DecodeChunkedEncoding(char* buff, ssize_t size, ssize_t& offset);

 public:
  int status_code_;

  Request();
  Request(const Request& obj);
  ~Request();
  Request& operator=(const Request& obj);
};

#endif
