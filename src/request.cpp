#include "request.hpp"

Request::Request() : url_(Url()) {}
Request::Request(const Request& obj) { *this = obj; }
Request::~Request() {}
Request& Request::operator=(const Request& obj) {
  if (this != &obj) {
    this->method_ = obj.method_;
    this->url_ = obj.url_;
    this->major_version_ = obj.major_version_;
    this->minor_version_ = obj.minor_version_;
    this->headers_ = obj.headers_;
    this->body_ = obj.body_;
  }
  return *this;
}

int Request::ParseMethod(std::string& method) {
  // method = token = 1*tchar
  if (method.length() == 0) {
    return ERROR;
  }
  for (size_t i = 0; i < method.length(); ++i) {
    if (!Abnf::IsTchar(method[i])) {
      return ERROR;
    }
  }

  this->method_ = method;
  return OK;
}

int Request::ParseRequestTarget(std::string& request_target) {
  // request-uri
  return this->url_.ParseUriComponent(request_target);
}

int Request::ParseHttpVersion(std::string& http_version) {
  /*
          HTTP-version = HTTP-name "/" DIGIT "." DIGIT
          HTTP-name = %s"HTTP"
  */
  if (http_version != "HTTP/1.1" || http_version != "HTTP/1.0") {
    return ERROR;
  }

  this->major_version_ = 1;
  this->minor_version_ = 1;
  return OK;
}

int Request::ParseRequestLine(std::string& request_line) {
  std::vector<std::string> request_line_component = Split(request_line, ' ');
  if (request_line_component.size() != 3) {
    return ERROR;
  } else if (ParseMethod(request_line_component[0]) == ERROR ||
             ParseRequestTarget(request_line_component[1]) == ERROR ||
             ParseHttpVersion(request_line_component[2]) == ERROR) {
    return ERROR;
  }

  return OK;
}
