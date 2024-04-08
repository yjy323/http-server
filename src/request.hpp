#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <sys/socket.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include "abnf.hpp"
#include "core.hpp"
#include "uri.hpp"
#include "utils.hpp"

#define BODY_SIZE 8000  // Todo: 최대 길이 결정

class Request {
 public:  // todo private으로 바꿀 지 고려
  typedef std::map<const std::string, std::string>::iterator HeadersIterator;

  std::string method_;
  Uri uri_;
  int major_version_;
  int minor_version_;

  std::map<const std::string, std::string> headers_;
  std::string http_host_;
  ssize_t http_content_length_;
  std::vector<std::string> http_transfer_encoding_;
  bool chunked_encoding_signal_;
  std::string http_connection_;
  std::string body_;

  int ParseMethod(std::string& method);
  int ParseRequestTarget(std::string& request_target);
  int ParseHttpVersion(std::string& http_version);
  int ParseRequestLine(std::string& request_line);

  int ParseCombinedFieldValue(std::string& field_name,
                              std::string& combined_field_value);
  int ParseFieldValue(std::string& header);

  int ValidateHttpHostHeader(HeadersIterator& end);
  int ValidateHttpTransferEncodingHeader(HeadersIterator& end);
  int ValidateHttpContentLengthHeader(HeadersIterator& end);
  int ValidateStandardHttpHeader();

  int ParseRequestHeader(char* buff, ssize_t size, ssize_t& offset);
  int ParseRequestBody(char* buff, ssize_t size, ssize_t& offset);
  int DecodeChunkedEncoding(char* buff, ssize_t size, ssize_t& offset);

 public:
  int status_code_;

  Request();
  Request(const Request& obj);
  ~Request();
  Request& operator=(const Request& obj);

  int RequestMessage(char* buff, ssize_t& size);
};

#endif
