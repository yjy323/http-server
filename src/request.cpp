#include "request.hpp"

int ProcessHttpHeaderHost(HeadersIn&, const std::string);
int ProcessHttpHeaderTransferEncoding(HeadersIn&, const std::string);
int ProcessHttpHeaderContentLength(HeadersIn&, const std::string);

int ProcessHttpHeaderHost(HeadersIn& headers, std::string value) {
  if (value.size() == 0 || !IsHost(value)) {
    return HTTP_BAD_REQUEST;
  } else if (headers.host_ != "") {
    return HTTP_BAD_REQUEST;
  } else {
    headers.host_ = value;
    return HTTP_OK;
  }
}
int ProcessHttpHeaderTransferEncoding(HeadersIn& headers, std::string value) {
  if (!IsToken(value)) {
    return HTTP_BAD_REQUEST;
  } else if (ToCaseInsensitive(Trim(value)) != "chunked") {
    return HTTP_NOT_IMPLEMENTED;
  } else {
    headers.transfer_encoding_ = value;
    headers.chuncked = true;
    return HTTP_OK;
  }
}

int ProcessHttpHeaderContentLength(HeadersIn& headers, std::string value) {
  char* end_ptr;

  ssize_t content_length_n = strtol(value.c_str(), &end_ptr, 10);
  if (end_ptr == value || *end_ptr != 0) {
    return HTTP_BAD_REQUEST;
  } else {
    headers.content_length_ = value;
    headers.content_length_n_ = content_length_n;
    return HTTP_OK;
  }
}

Request::Request()
    : method_(""), uri_(Uri()), headers_in_(HeadersIn()), body_("") {}
Request::Request(const Request& obj) { *this = obj; }
Request::~Request() {}
Request& Request::operator=(const Request& obj) {
  if (this != &obj) {
    this->method_ = obj.method_;
    this->uri_ = obj.uri_;
    this->headers_in_ = obj.headers_in_;
    this->body_ = obj.body_;
  }
  return *this;
}

const std::string& Request::method() const { return this->method_; }
const Uri& Request::uri() const { return this->uri_; }
const HeadersIn& Request::headers_in() const { return this->headers_in_; }
const std::string& Request::body() const { return this->body_; }

int Request::ParseRequestLine(std::string& request_line) {
  std::vector<std::string> request_line_component = Split(request_line, ' ');
  if (request_line_component.size() != 3) {
    return HTTP_BAD_REQUEST;
  } else {
    std::string method = request_line_component[0];
    std::string request_target = request_line_component[1];
    std::string http_version = request_line_component[2];
    if (method.length() == 0 || !IsToken(method)) {
      return HTTP_BAD_REQUEST;
    }
    if (this->uri_.ParseUriComponent(request_target) == HTTP_BAD_REQUEST) {
      return HTTP_BAD_REQUEST;
    }
    if (http_version != "HTTP/1.1" && http_version != "HTTP/1.0") {
      return HTTP_BAD_REQUEST;
    }
    this->method_ = method;
    return HTTP_OK;
  }
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

  field_name = ToCaseInsensitive(field_line.substr(0, delimiter_pos));
  if (field_name.length() == 0 || !IsToken(field_name)) {
    return HTTP_BAD_REQUEST;
  }

  field_value = Trim(field_line.substr(delimiter_pos + 1));
  for (size_t i = 0; i < field_value.length(); i++) {
    unsigned char c = field_value[i];
    if (IsVchar(c) || IsObsText(c) || IsWhiteSpace(c)) {
      continue;
    } else {
      return HTTP_BAD_REQUEST;
    }
  }

  if (field_name == "host") {
    ProcessHttpHeaderHost(headers_in_, field_value);
  } else if (field_name == "connection") {
  } else if (field_name == "if-modified_since") {
  } else if (field_name == "if-unmodified_since") {
  } else if (field_name == "if-match") {
  } else if (field_name == "if-none_match") {
  } else if (field_name == "user-agent") {
  } else if (field_name == "referer") {
  } else if (field_name == "content-length") {
    ProcessHttpHeaderContentLength(headers_in_, field_value);
  } else if (field_name == "content-range") {
  } else if (field_name == "content-type") {
  } else if (field_name == "range") {
  } else if (field_name == "if-range") {
  } else if (field_name == "transfer-encoding") {
    ProcessHttpHeaderTransferEncoding(headers_in_, field_value);
  } else if (field_name == "te") {
  } else if (field_name == "expect") {
  } else if (field_name == "upgrade") {
  }

  InsertHeader(headers_in_.headers_, field_name, field_value);
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
  std::string buff_str(buff);
  std::string line;
  size_t crlf_pos;

  bool start_line_flag = true;

  while (offset < size) {
    crlf_pos = buff_str.find(CRLF);
    if (crlf_pos == buff_str.npos) {
      // CRLF가 존재하지 않는 요청 해더
      return HTTP_BAD_REQUEST;
    } else {
      line = buff_str.substr(0, crlf_pos);
      buff_str = buff_str.substr(crlf_pos + 2);
      offset = line.size() + 2;

      if (start_line_flag && line.empty()) {
        continue;
      } else if (start_line_flag) {
        if (ParseRequestLine(line) == HTTP_BAD_REQUEST) {
          return HTTP_BAD_REQUEST;
        } else {
          start_line_flag = false;
          continue;
        }
      } else if (line.empty()) {
        return HTTP_OK;
      } else {
        if (ParseFieldValue(line) == HTTP_BAD_REQUEST) {
          return HTTP_BAD_REQUEST;
        }
      }
    }
  }
  return HTTP_BAD_REQUEST;
}

/*
        int Request::DecodeChunkedEncoding(char* buff, ssize_t size, ssize_t&
                offset)
        {}
*/

int Request::ParseRequestBody(char* buff, ssize_t size, ssize_t& offset) {
  if (headers_in_.chuncked) {
  } else {
    // DecodeChunkedEncoding(buff, size, offset);
    for (; offset < size; ++offset) {
      char c = buff[offset];
      if (!IsOctet(c)) {
        return HTTP_BAD_REQUEST;
      }
      this->body_ = std::string(buff);
    }
    return HTTP_OK;
  }
}
