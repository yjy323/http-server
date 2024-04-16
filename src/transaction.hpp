#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP

#include <sys/stat.h>

#include <ctime>
#include <fstream>

#include "cgi.hpp"
#include "configuration.hpp"
#include "http.hpp"
#include "request.hpp"
#include "utils.hpp"

class Transaction {
 public:
  typedef ServerConfiguration::LocationConfiguration Configuration;
  enum ResourceType { kFile, kDirectory };

  Transaction();
  Transaction(const Transaction& obj);
  ~Transaction();
  Transaction& operator=(const Transaction& obj);

  int ParseRequestHeader(const char* buff, ssize_t size, ssize_t& offset);
  int ParseRequestBody(char* buff, ssize_t size, ssize_t& offset);

  int HttpGetMethod();
  int HttpPostMethod();
  int HttpDeleteMethod();

  void SetConfiguration(const ServerConfiguration&);
  void SetTargetResource();
  bool IsAllowedMethod(const char*);
  bool IsRedirectedUri();

  const Configuration& config() const;
  const Uri& uri() const;
  const std::string& method() const;
  const HeadersIn& headers_in() const;
  const HeadersOut& headers_out() const;
  const std::string& body() const;
  int status_code();

 private:
  int ParseRequestLine(std::string& request_line);
  int ParseFieldValue(std::string& header);
  int DecodeChunkedEncoding(char* buff, ssize_t size, ssize_t& offset);

  Configuration config_;
  Uri uri_;

  std::string method_;
  HeadersIn headers_in_;
  HeadersOut headers_out_;
  std::string body_;

  int status_code_;
  std::string entity_body_;

  std::string target_resource_;
  std::string target_resource_extension_;
  ResourceType target_resource_type_;
};

#endif
