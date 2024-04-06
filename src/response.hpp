#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sys/stat.h>

#include <ctime>
#include <fstream>

#include "configuration.hpp"
#include "request.hpp"
#include "utils.hpp"

class Response {
 public:
  typedef ServerConfiguration::LocationConfiguration LocationConfiguration;
  typedef std::map<const std::string,
                   const LocationConfiguration>::const_iterator LocConfIterator;

  enum ResourceType { kFile, kDirectory };

  Response();
  Response(const Response& obj);
  ~Response();
  Response& operator=(const Response& obj);

  void FindResourceConfiguration();

  int HttpTransaction();
  int HttpGetMethod();
  int HttpPostMethod();
  int HttpDeleteMethod();

  bool IsAllowedMethod(const char* method);

  Request request_;
  ServerConfiguration server_conf_;
  LocationConfiguration loc_conf_;
  std::string request_target_;
  std::string target_resource_;
  ResourceType target_resource_type_;
};

#endif
