#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

#include "abnf.hpp"
#include "http.hpp"
#include "uri.hpp"
#include "utils.hpp"

class Request {
 public:
  Request();
  Request(const Request& obj);
  ~Request();
  Request& operator=(const Request& obj);

  int ParseRequestHeader(const char* buff, ssize_t size, ssize_t& offset);
  int ParseRequestBody(char* buff, ssize_t size, ssize_t& offset);

  const std::string& method() const;
  const Uri& uri() const;
  const HeadersIn& headers_in() const;
  const std::string& body() const;

 private:
  std::string method_;
  Uri uri_;

  HeadersIn headers_in_;
  std::string body_;

  int ParseRequestLine(std::string& request_line);
  int ParseFieldValue(std::string& header);

  int DecodeChunkedEncoding(char* buff, ssize_t size, ssize_t& offset);
};

#endif
