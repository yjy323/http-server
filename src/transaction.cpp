#include "transaction.hpp"

#include "abnf.hpp"
#include "utils.hpp"

/*
        비멤버 함수
*/
char* SetEnv(const char*, const char*);

/*
        멤버함수
*/
Transaction::Transaction()
    : config_(),
      method_(""),
      uri_(),
      headers_in_(),
      body_in_(""),
      status_code_(HTTP_OK),
      target_resource_(""),
      headers_out_(),
      entity_(),
      cgi_() {}

Transaction::Transaction(const Transaction& obj) { *this = obj; }

Transaction::~Transaction() {}

Transaction& Transaction::operator=(const Transaction& obj) {
  if (this != &obj) {
    this->method_ = obj.method_;
    this->uri_ = obj.uri_;
    this->headers_in_ = obj.headers_in_;
    this->body_in_ = obj.body_in_;

    this->config_ = obj.config_;
    this->status_code_ = obj.status_code_;
    this->headers_out_ = obj.headers_out_;
  }
  return *this;
}

/*
        Getter
*/
const std::string& Transaction::method() const { return this->method_; }
const Uri& Transaction::uri() const { return this->uri_; }
const HeadersIn& Transaction::headers_in() const { return this->headers_in_; }
const std::string& Transaction::body() const { return this->body_in_; }
const Transaction::Configuration& Transaction::config() const {
  return this->config_;
}
int Transaction::status_code() { return this->status_code_; }
const HeadersOut& Transaction::headers_out() const {
  return this->headers_out_;
}

int Transaction::ParseRequestLine(std::string& request_line) {
  std::vector<std::string> request_line_component = Split(request_line, ' ');
  if (request_line_component.size() != 3) {
    return HTTP_BAD_REQUEST;
  }
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

int Transaction::ParseFieldValue(std::string& field_line) {
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
    IF_BAD_HEADER_THEN_RETURN(HttpHost(headers_in_, field_value));
  } else if (field_name == "connection") {
  } else if (field_name == "if-modified_since") {
  } else if (field_name == "if-unmodified_since") {
  } else if (field_name == "if-match") {
  } else if (field_name == "if-none_match") {
  } else if (field_name == "user-agent") {
  } else if (field_name == "referer") {
  } else if (field_name == "content-length") {
    IF_BAD_HEADER_THEN_RETURN(HttpContentLength(headers_in_, field_value));
  } else if (field_name == "content-range") {
  } else if (field_name == "content-type") {
  } else if (field_name == "range") {
  } else if (field_name == "if-range") {
  } else if (field_name == "transfer-encoding") {
    IF_BAD_HEADER_THEN_RETURN(HttpTransferEncoding(headers_in_, field_value));
  } else if (field_name == "te") {
  } else if (field_name == "expect") {
  } else if (field_name == "upgrade") {
  }
  HttpInsertHeader(headers_in_.headers_, field_name, field_value);

  return HTTP_OK;
}
/*
        int Transaction::DecodeChunkedEncoding(char* buff, ssize_t size,
   ssize_t& offset)
        {}
*/
int Transaction::ParseRequestHeader(const char* buff, ssize_t size,
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
    }

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
  return HTTP_BAD_REQUEST;
}
int Transaction::ParseRequestBody(char* buff, ssize_t size, ssize_t& offset) {
  if (headers_in_.chuncked) {
    return HTTP_BAD_REQUEST;
  } else {
    // DecodeChunkedEncoding(buff, size, offset);
    for (; offset < size; ++offset) {
      char c = buff[offset];
      if (!IsOctet(c)) {
        return HTTP_BAD_REQUEST;
      }
      this->body_in_ = std::string(buff);
    }
    return HTTP_OK;
  }
}

const Transaction::Configuration& Transaction::GetConfiguration(
    const ServerConfiguration& server_config) {
  typedef std::map<std::string, Configuration>::const_iterator
      ConfigurationIterator;

  size_t max_match_pos = 0;

  for (ConfigurationIterator it = server_config.location().begin();
       it != server_config.location().end(); ++it) {
    std::string key = it->first;
    size_t match_pos = uri_.request_target().find(key);
    if (match_pos == uri_.request_target().npos && max_match_pos != 0) {
      break;
    } else if (match_pos == 0 && max_match_pos < key.length()) {
      config_ = it->second;
      max_match_pos = key.length();
    }
  }

  if (max_match_pos == 0) {
    std::set<std::string> default_allowed_method;
    default_allowed_method.insert("GET");

    config_ = Transaction::Configuration(
        server_config.error_page(), server_config.client_max_body_size(),
        server_config.root(), server_config.auto_index(), server_config.index(),
        default_allowed_method, "", "");
  }
  return config_;
}

std::string Transaction::GetTargetResource() {
  return target_resource_ = "." + config_.root() + uri_.request_target();
}

bool Transaction::IsRedirectedUri() {
  std::string request_target = uri_.request_target();
  if (uri_.request_target() == config_.return_uri()) {
    return true;
  } else {
    return false;
  }
}

bool Transaction::IsAllowedMethod() {
  if (config_.allowed_method().find(method_) !=
      config_.allowed_method().end()) {
    return true;
  } else {
    return false;
  }
}

void Transaction::Test() {
  if (IsRedirectedUri()) {
    HTTP_MOVED_PERMANENTLY;
  }

  if (!IsAllowedMethod()) {
    HTTP_NOT_ALLOWED;
  }

  if (method_ == HTTP_GET_METHOD) {
    if (!Entity::IsFileExist(target_resource_.c_str())) HTTP_NOT_FOUND;
    entity_ = Entity(target_resource_);

    if (Cgi::IsSupportedCgi(entity_.extension().c_str()) &&
        Entity::IsFileExecutable(target_resource_.c_str())) {
      cgi_.TurnOn();
    } else if (Entity::IsFileReadable(target_resource_.c_str())) {
      entity_.ReadFile();
    } else {
      HTTP_FORBIDDEN;
    }
  }

  if (method_ == HTTP_POST_METHOD) {
    if (!Entity::IsFileExist(target_resource_.c_str())) HTTP_NOT_FOUND;

    entity_ = Entity(target_resource_);
    if (Cgi::IsSupportedCgi(entity_.extension().c_str()) &&
        Entity::IsFileExecutable(target_resource_.c_str())) {
      cgi_.TurnOn();
    } else {
      HTTP_FORBIDDEN;
    }
  }

  if (method_ == HTTP_DELETE_METHOD) {
    if (!Entity::IsFileExist(target_resource_.c_str())) HTTP_NO_CONTENT;
    if (!Entity::IsFileExecutable(target_resource_.c_str())) HTTP_FORBIDDEN;
  }
}

char* SetEnv(const char* key, const char* value) {
  char* env = new char[std::strlen(key) + std::strlen(value) + 1];
  std::strcpy(env, key);
  std::strcat(env, value);
  return (char* const)env;
}

void Transaction::SetCgiEnv() {
  if (method_ == HTTP_GET_METHOD) {
    cgi_.envp().push_back(SetEnv("REQUEST_METHOD=", "GET"));
    cgi_.envp().push_back(SetEnv("QUERY_STRING=", uri_.query_string().c_str()));

  } else if (method_ == HTTP_POST_METHOD) {
    cgi_.envp().push_back(SetEnv("REQUEST_METHOD=", "POST"));
    cgi_.envp().push_back(
        SetEnv("CONTENT_LENGTH=", headers_in_.content_length_.c_str()));
  }
}

void Transaction::FreeCgiEnv() {
  typedef std::vector<char* const>::const_iterator ConstIterator;
  for (ConstIterator it = cgi_.envp().begin(); it != cgi_.envp().end(); ++it) {
    delete *it;
  }
}
