#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sys/stat.h>

#include <ctime>
#include <fstream>

#include "cgi.hpp"
#include "configuration.hpp"
#include "request.hpp"
#include "utils.hpp"

class Response {
 public:
  typedef ServerConfiguration::LocationConfiguration LocationConfiguration;
  typedef std::map<std::string, LocationConfiguration>::const_iterator
      LocConfIterator;

  enum ResourceType { kFile, kDirectory };
  Response(const Request&, const ServerConfiguration&);
  Response(const Response& obj);
  Response();
  ~Response();
  Response& operator=(const Response& obj);

  void FindResourceConfiguration();

  int HttpTransaction();
  int HttpGetMethod();
  int HttpPostMethod();
  int HttpDeleteMethod();

  void SetStatusLine();
  int SetResponseHeader();

  int GetMimeType();
  int GetStaticFile();
  int GetCgiScript();

  bool IsAllowedMethod(const char*);

  Request request_;
  ServerConfiguration server_conf_;

  LocationConfiguration loc_conf_;
  std::string request_target_;
  std::string target_resource_;
  std::string target_resource_extension_;
  ResourceType target_resource_type_;

  std::string status_line_;
  std::string response_header_;
  std::string response_body_;
  std::string response_message_;
};

#endif
