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
  // method = token
  if (method.length() == 0 || Abnf::IsToken(method) == ERROR) {
    return ERROR;
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

int Request::ParseHttpHeader(std::string& header) {
  /*
                field-line = field-name ":" OWS field-value OWS
                field-name = token

                field-value = *field-content
                field-content = field-vchar
                        [ 1*( SP / HTAB / field-vchar ) field-vchar ]
                field-vchar = VCHAR / obs-text
                VCHAR =  %x21-7E ; visible (printing) characters
                obs-text = %x80-FF
  */
  std::string field_name;
  std::string field_value;
  size_t delimiter_pos = header.find(':');
  if (delimiter_pos == std::string::npos) {
    return ERROR;
  }

  field_name = header.substr(0, delimiter_pos);
  if (field_name.length() == 0 || Abnf::IsToken(field_name) == ERROR) {
    return ERROR;
  }

  field_value = Trim(field_value);
  for (size_t i = 0; i < field_value.length(); i++) {
    unsigned char c = field_value[i];
    if (Abnf::IsVchar(c) || Abnf::IsObsText(c) || Abnf::IsWhiteSpace(c)) {
      continue;
    } else {
      return ERROR;
    }
  }
  this->headers_.insert(std::make_pair(field_name, field_value));
  return OK;
}
