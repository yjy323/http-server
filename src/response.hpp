#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sys/stat.h>

#include <ctime>
#include <fstream>

#include "cgi.hpp"
#include "configuration.hpp"
#include "http.hpp"
#include "request.hpp"
#include "utils.hpp"

class Response {
 public:
  typedef ServerConfiguration::LocationConfiguration Configuration;
  enum ResourceType { kFile, kDirectory };
  Response(const Request&);
  Response(const Response& obj);
  Response();
  ~Response();
  Response& operator=(const Response& obj);

  int HttpGetMethod();
  int HttpPostMethod();
  int HttpDeleteMethod();

  void SetConfiguration(const ServerConfiguration&);
  void SetTargetResource();

  bool IsAllowedMethod(const char*);
  bool IsRedirectedUri();

  Request request_;

  int status_code_;
  HeadersOut headers_;
  std::string body_;

  Configuration config_;

  std::string target_resource_;
  std::string target_resource_extension_;
  ResourceType target_resource_type_;
};

#endif
