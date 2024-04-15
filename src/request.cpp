#include "request.hpp"

Request::Request() : uri_(Uri()), http_content_length_(-1) {}
Request::Request(const Request& obj) { *this = obj; }
Request::~Request() {}
Request& Request::operator=(const Request& obj) {
  if (this != &obj) {
    this->method_ = obj.method_;
    this->uri_ = obj.uri_;
    this->major_version_ = obj.major_version_;
    this->minor_version_ = obj.minor_version_;
    this->headers_ = obj.headers_;
    this->http_host_ = obj.http_host_;
    this->http_content_length_ = obj.http_content_length_;
    this->http_transfer_encoding_ = obj.http_transfer_encoding_;
    this->chunked_encoding_signal_ = obj.chunked_encoding_signal_;
    this->http_connection_ = obj.http_connection_;
    this->body_ = obj.body_;
  }
  return *this;
}

int Request::ParseMethod(std::string& method) {
  // method = token

  if (method.length() == 0 || !Abnf::IsToken(method)) {
    return HTTP_BAD_REQUEST;
  }
  this->method_ = method;
  return HTTP_OK;
}

int Request::ParseRequestTarget(std::string& request_target) {
  // request-uri
  return this->uri_.ParseUriComponent(request_target);
}

int Request::ParseHttpVersion(std::string& http_version) {
  /*
          HTTP-version = HTTP-name "/" DIGIT "." DIGIT
          HTTP-name = %s"HTTP"
  */
  if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
    return HTTP_BAD_REQUEST;
  }

  this->major_version_ = 1;
  this->minor_version_ = 1;
  return HTTP_OK;
}

int Request::ParseRequestLine(std::string& request_line) {
  std::vector<std::string> request_line_component = Split(request_line, ' ');
  if (request_line_component.size() != 3) {
    return HTTP_BAD_REQUEST;
  } else if (ParseMethod(request_line_component[0]) == HTTP_BAD_REQUEST ||
             ParseRequestTarget(request_line_component[1]) ==
                 HTTP_BAD_REQUEST ||
             ParseHttpVersion(request_line_component[2]) == HTTP_BAD_REQUEST) {
    return HTTP_BAD_REQUEST;
  }
  return HTTP_OK;
}

int Request::ParseCombinedFieldValue(std::string& field_name,
                                     std::string& combined_field_value) {
  if (field_name == "host") {
    return HTTP_BAD_REQUEST;
  } else if (field_name == "content-length") {
    return HTTP_BAD_REQUEST;
  } else if (field_name == "transfer-encoding") {
    return HTTP_BAD_REQUEST;
  }
  this->headers_[field_name] = combined_field_value;
  return HTTP_OK;
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
    return HTTP_BAD_REQUEST;
  }

  field_name = field_line.substr(0, delimiter_pos);
  field_name = ToCaseInsensitive(field_name);
  if (field_name.length() == 0 || !Abnf::IsToken(field_name)) {
    return HTTP_BAD_REQUEST;
  }

  field_value = Trim(field_line.substr(delimiter_pos + 1));
  for (size_t i = 0; i < field_value.length(); i++) {
    unsigned char c = field_value[i];
    if (Abnf::IsVchar(c) || Abnf::IsObsText(c) || Abnf::IsWhiteSpace(c)) {
      continue;
    } else {
      return HTTP_BAD_REQUEST;
    }
  }

  HeadersIterator field_iter = this->headers_.find(field_name);
  if (field_iter != this->headers_.end()) {
    std::string combined_field_value = field_iter->second + ", " + field_value;
    if (ParseCombinedFieldValue(field_name, combined_field_value) ==
        HTTP_BAD_REQUEST) {
      return HTTP_BAD_REQUEST;
    }
  } else {
    this->headers_.insert(std::make_pair(field_name, field_value));
  }
  return HTTP_OK;
}

int Request::ValidateHttpHostHeader(HeadersIterator& end) {
  std::string field_value;
  char* end_ptr;

  HeadersIterator it = this->headers_.find("host");
  if (it == end) {
    return HTTP_BAD_REQUEST;
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
        return HTTP_BAD_REQUEST;
      }

      field_value = field_value.substr(0, delimiter_pos);
    }
    if (field_value.length() == 0 || !Abnf::IsHost(field_value)) {
      return this->status_code_ = HTTP_BAD_REQUEST;
    }
  }

  return HTTP_OK;
}

int Request::ValidateHttpTransferEncodingHeader(HeadersIterator& end) {
  HeadersIterator it = this->headers_.find("transfer-encoding");
  std::string field_value;

  if (it != end) {
    field_value = it->second;
    std::vector<std::string> buffer = Split(field_value, ',');

    std::vector<std::string>::iterator buffer_it = buffer.begin();
    std::vector<std::string>::iterator buffer_end = buffer.end();

    std::string token;
    for (; buffer_it < buffer_end; ++buffer_it) {
      token = Trim(*buffer_it);
      token = token.substr(0, token.find(';'));
      token = ToCaseInsensitive(token);
      if (!Abnf::IsToken(token)) {
        return HTTP_BAD_REQUEST;
      } else if (token != "chunked") {
        // chunked 외의 전송 코딩은 지원하지 않는다.
        // 확장성을 위해 vector<string transfer-coding>으로 관리한다.
        return HTTP_NOT_IMPLEMENTED;
      } else {
        chunked_encoding_signal_ = true;
        this->http_transfer_encoding_.push_back(token);
      }
    }
  }
  return HTTP_OK;
}
int Request::ValidateHttpContentLengthHeader(HeadersIterator& end) {
  HeadersIterator it = this->headers_.find("content-length");
  std::string field_value;
  char* end_ptr;

  if (it != end) {
    field_value = it->second;
    long content_length = strtol(field_value.c_str(), &end_ptr, 10);
    if (end_ptr == field_value || *end_ptr != 0) {
      return HTTP_BAD_REQUEST;
    }
    this->http_content_length_ = content_length;
  }
  return HTTP_OK;
}

int Request::ValidateStandardHttpHeader() {
  HeadersIterator end = this->headers_.end();

  if (ValidateHttpHostHeader(end) == HTTP_BAD_REQUEST ||
      ValidateHttpTransferEncodingHeader(end) == HTTP_BAD_REQUEST ||
      ValidateHttpContentLengthHeader(end) == HTTP_BAD_REQUEST) {
    return HTTP_BAD_REQUEST;
  }
  return HTTP_OK;
}

int Request::ParseRequestHeader(const char* buff, ssize_t size,
                                ssize_t& offset) {
  /*
          HTTP-message = start-line CRLF
                        *( field-line CRLF )
                        CRLF
                        [ message-body ]
  */
  std::stringstream ss;
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
          if (this->ParseRequestLine(line) != HTTP_OK) {
            return this->status_code_ = HTTP_BAD_REQUEST;
          }
          start_line_flag = false;
        }
      } else {
        if (line.empty()) {
          break;
        } else {
          if (this->ParseFieldValue(line) != HTTP_OK) {
            return this->status_code_ = HTTP_BAD_REQUEST;
          }
        }
      }
    }
  }

  if (this->ValidateStandardHttpHeader() != HTTP_OK) {
    return this->status_code_;
  }
  return HTTP_OK;
}

// int Request::DecodeChunkedEncoding(char* buff, ssize_t size, ssize_t& offset)
// {}

int Request::ParseRequestBody(char* buff, ssize_t size, ssize_t& offset) {
  std::stringstream ss;
  if (chunked_encoding_signal_) {
  } else {
    for (; offset < size; ++offset) {
      char c = buff[offset];
      if (Abnf::IsOctet(c)) {
        ss << c;
      } else {
        return this->status_code_ = HTTP_BAD_REQUEST;
      }
    }
    this->body_ += ss.str();
  }
  return HTTP_OK;
}

int Request::RequestMessage(char* buff, ssize_t& size) {
  ssize_t offset = 0;
  if (ParseRequestHeader(buff, size, offset) != HTTP_OK) {
    return this->status_code_;
  }

  if (chunked_encoding_signal_ & (http_content_length_ > -1)) {
    return HTTP_BAD_REQUEST;
  } else if (chunked_encoding_signal_ | (http_content_length_ > -1)) {
    if (ParseRequestBody(buff, size, ++offset) != HTTP_OK) {
      return this->status_code_;
    }
  }
  return this->status_code_;
}
