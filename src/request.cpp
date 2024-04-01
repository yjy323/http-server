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

  if (method.length() == 0 || !Abnf::IsToken(method)) {
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
  if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
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

int Request::ParseCombinedFieldValue(std::string& field_name,
                                     std::string& combined_field_value) {
  if (field_name == "host") {
    return ERROR;
  } else if (field_name == "content-length") {
    return ERROR;
  } else if (field_name == "transfer-encoding") {
    return ERROR;
  }
  this->headers_[field_name] = combined_field_value;
  return OK;
}

int Request::ParseFieldValue(std::string& field_line) {
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
  size_t delimiter_pos = field_line.find(':');
  if (delimiter_pos == std::string::npos) {
    return ERROR;
  }

  field_name = field_line.substr(0, delimiter_pos);
  ToCaseInsensitve(field_name);
  if (field_name.length() == 0 || !Abnf::IsToken(field_name)) {
    return ERROR;
  }

  field_value = Trim(field_line.substr(delimiter_pos + 1));
  for (size_t i = 0; i < field_value.length(); i++) {
    unsigned char c = field_value[i];
    if (Abnf::IsVchar(c) || Abnf::IsObsText(c) || Abnf::IsWhiteSpace(c)) {
      continue;
    } else {
      return ERROR;
    }
  }

  HeadersIterator field_iter = this->headers_.find(field_name);
  if (field_iter != this->headers_.end()) {
    std::string combined_field_value = field_iter->second + ", " + field_value;
    if (ParseCombinedFieldValue(field_name, combined_field_value) == ERROR) {
      return ERROR;
    }
  } else {
    this->headers_.insert(std::make_pair(field_name, field_value));
  }
  return OK;
}

int Request::ParseStandardHeader() {
  HeadersIterator end = this->headers_.end();
  std::string field_value;
  char* end_ptr;

  HeadersIterator it = this->headers_.find("host");
  if (it == end) {
    return ERROR;
  } else {
    field_value = it->second;
    this->http_host_ = field_value;

    size_t delimiter_pos;
    std::string sub_component;

    delimiter_pos = field_value.find(':');
    if (delimiter_pos != std::string::npos) {
      // port = *DIGIT
      sub_component = field_value.substr(delimiter_pos + 1);
      long port = strtol(sub_component.c_str(), &end_ptr, 10);
      if (end_ptr == sub_component || *end_ptr != 0 || port < 0 ||
          port > 65535) {
        return ERROR;
      }

      field_value = field_value.substr(0, delimiter_pos);
    }

    if (!Abnf::IsHost(field_value)) {
      return ERROR;
    }
  }

  it = this->headers_.find("transfer-encoding");
  if (it != end) {
    field_value = it->second;
    std::vector<std::string> buffer = Split(field_value, ',');

    std::vector<std::string>::iterator buffer_it = buffer.begin();
    std::vector<std::string>::iterator buffer_end = buffer.end();

    std::string token;
    for (; buffer_it < buffer_end; ++buffer_it) {
      token = Trim(*buffer_it);
      token = token.substr(0, token.find(';'));
      ToCaseInsensitve(token);
      this->http_transfer_encoding_.push_back(token);
    }
  }

  it = this->headers_.find("content-length");
  if (it != end) {
    if (this->headers_.find("transfer-encoding") != end) {
      return ERROR;
    }
    field_value = it->second;
    long content_length = strtol(field_value.c_str(), &end_ptr, 10);
    if (end_ptr == field_value || *end_ptr != 0) {
      return ERROR;
    }
    this->http_content_length_ = content_length;
  }
  return OK;
}

int Request::ReceiveRequest(char* buff, ssize_t size) {
  /*
          HTTP-message = start-line CRLF
                        *( field-line CRLF )
                        CRLF
                        [ message-body ]
  */
  std::stringstream ss;
  ssize_t offset = 0;
  bool start_line_flag = true;

  for (; offset < size; ++offset) {
    char c = buff[offset];
    ss << c;
    if (c == '\n' && (offset > 0 && buff[offset - 1] == '\r')) {
      std::string line = ss.str();
      ss.str(std::string());
      line = line.substr(0, line.length() - 2);

      if (start_line_flag) {
        if (line.empty()) {
          continue;
        } else {
          if (this->ParseRequestLine(line) == ERROR) {
            return ERROR;
          }
          start_line_flag = false;
        }
      } else {
        if (line.empty()) {
          break;
        } else if (this->ParseFieldValue(line) == ERROR) {
          return ERROR;
        }
      }
    }
  }
  if (ParseStandardHeader() == ERROR) {
    return ERROR;
  }

  return OK;
}

const std::string& Request::get_method() const { return this->method_; }

const Url& Request::get_url() const { return this->url_; }

int Request::get_major_version() const { return this->major_version_; }

int Request::get_minor_version() const { return this->minor_version_; }

const std::map<const std::string, std::string>& Request::get_headers() const {
  return this->headers_;
}

const std::vector<std::string> Request::get_http_transfer_encoding() const {
  return this->http_transfer_encoding_;
}

int Request::get_http_content_length() const { return this->http_content_length_; }
const std::string& Request::get_body() const { return this->body_; }
