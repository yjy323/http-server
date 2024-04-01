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
#define BODY_SIZE 8000  // Todo: 최대 길이 결정

class Request {
 private:
  typedef std::map<const std::string, std::string>::iterator HeadersIterator;

  std::string method_;
  Url url_;
  int major_version_;
  int minor_version_;

  std::map<const std::string, std::string> headers_;
  std::string http_host_;
  size_t http_content_length_;
  std::vector<std::string> http_transfer_encoding_;
  std::string http_connection_;
  std::string body_;

  int ParseMethod(std::string& method);
  int ParseRequestTarget(std::string& request_target);
  int ParseHttpVersion(std::string& http_version);
  int ParseRequestLine(std::string& request_line);

  int ParseCombinedFieldValue(std::string& field_name,
                              std::string& combined_field_value);
  int ParseFieldValue(std::string& header);

  int ParseStandardHeader();

 public:
  Request();
  Request(const Request& obj);
  ~Request();
  Request& operator=(const Request& obj);

  int ReceiveRequest(char* buff, ssize_t size);

  const std::string& get_method() const;
  const Url& get_url() const;
  int get_major_version() const;
  int get_minor_version() const;
  const std::map<const std::string, std::string>& get_headers() const;

  const std::vector<std::string> get_http_transfer_encoding() const;
  int get_http_content_length() const;

  const std::string& get_body() const;
};

#endif
