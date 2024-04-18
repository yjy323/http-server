#include "transaction.hpp"

#include "abnf.hpp"
#include "utils.hpp"

#define SERVER_VERSION "1.0"
#define RETURN_STATUS_CODE return status_code_ =

#define IF_BAD_HEADER_THEN_RETURN(HeaderFunc) \
  if (HeaderFunc == HTTP_BAD_REQUEST) {       \
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;      \
  }

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
      http_version_(HTTP_1_1),
      server_version_(SERVER_VERSION),
      status_code_(HTTP_OK),
      target_resource_(""),
      headers_out_(),
      body_out_(""),
      entity_(),
      cgi_() {
  headers_out_.server = "webserv/" SERVER_VERSION;
}

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
    this->body_out_ = obj.body_out_;
    this->entity_ = obj.entity_;
    this->cgi_ = obj.cgi_;
  }
  return *this;
}

/*
        Getter
*/
const std::string& Transaction::method() const { return this->method_; }
const Uri& Transaction::uri() const { return this->uri_; }
const Entity& Transaction::entity() const { return this->entity_; }
Entity& Transaction::entity() { return this->entity_; }
const Cgi& Transaction::cgi() const { return this->cgi_; }
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
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }
  std::string method = request_line_component[0];
  std::string request_target = request_line_component[1];
  std::string http_version = request_line_component[2];
  if (method.length() == 0 || !IsToken(method)) {
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }
  if (this->uri_.ParseUriComponent(request_target) == HTTP_BAD_REQUEST) {
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }
  if (http_version != HTTP_1_1 && http_version != HTTP_1_0) {
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }
  this->method_ = method;
  RETURN_STATUS_CODE HTTP_OK;
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
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }

  field_name = ToCaseInsensitive(field_line.substr(0, delimiter_pos));
  if (field_name.length() == 0 || !IsToken(field_name)) {
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }

  field_value = Trim(field_line.substr(delimiter_pos + 1));
  for (size_t i = 0; i < field_value.length(); i++) {
    unsigned char c = field_value[i];
    if (IsVchar(c) || IsObsText(c) || IsWhiteSpace(c)) {
      continue;
    } else {
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
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

  RETURN_STATUS_CODE HTTP_OK;
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
      // return status_code_ = HTTP_BAD_REQUEST;
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
    }

    line = buff_str.substr(0, crlf_pos);
    buff_str = buff_str.substr(crlf_pos + 2);
    offset = line.size() + 2;

    if (start_line_flag && line.empty()) {
      continue;
    } else if (start_line_flag) {
      if (ParseRequestLine(line) == HTTP_BAD_REQUEST) {
        RETURN_STATUS_CODE HTTP_BAD_REQUEST;
      } else {
        start_line_flag = false;
        continue;
      }
    } else if (line.empty()) {
      RETURN_STATUS_CODE HTTP_OK;
    } else {
      if (ParseFieldValue(line) == HTTP_BAD_REQUEST) {
        RETURN_STATUS_CODE HTTP_BAD_REQUEST;
      }
    }
  }
  RETURN_STATUS_CODE HTTP_BAD_REQUEST;
}
int Transaction::ParseRequestBody(char* buff, ssize_t size, ssize_t& offset) {
  if (headers_in_.chuncked) {
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  } else {
    // DecodeChunkedEncoding(buff, size, offset);
    for (; offset < size; ++offset) {
      char c = buff[offset];
      if (!IsOctet(c)) {
        RETURN_STATUS_CODE HTTP_BAD_REQUEST;
      }
      this->body_in_ = std::string(buff);
    }
    RETURN_STATUS_CODE HTTP_OK;
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

void Transaction::SetEntityHeaders() {
  body_out_ = entity_.body();
  headers_out_.content_type = entity_.mime_type();
  headers_out_.content_length = entity_.length();
  headers_out_.last_modified = entity_.modified_s();
}

int Transaction::HttpProcess() {
  uri_.ReconstructTargetUri(headers_in_.host);
  if (uri_.request_target() == config_.return_uri()) {
    headers_out_.location = config_.return_uri();
    RETURN_STATUS_CODE HTTP_MOVED_PERMANENTLY;
  }

  target_resource_ = "." + config_.root() + uri_.request_target();

  if (config_.allowed_method().find(method_) ==
      config_.allowed_method().end()) {
    typedef std::set<std::string>::const_iterator METHOD_ITER;
    size_t method_cnt = config_.allowed_method().size();
    METHOD_ITER method = config_.allowed_method().begin();
    headers_out_.allow = "";
    for (size_t i = 0; i < method_cnt; ++i) {
      headers_out_.allow += *(method++) + (i < method_cnt - 1 ? ", " : "");
    }
    RETURN_STATUS_CODE HTTP_NOT_ALLOWED;
  }

  if (method_ == HTTP_GET_METHOD) {
    RETURN_STATUS_CODE HttpGet();
  }

  if (method_ == HTTP_POST_METHOD) {
    RETURN_STATUS_CODE HttpPost();
  }

  if (method_ == HTTP_DELETE_METHOD) {
    RETURN_STATUS_CODE HttpGet();
  }

  RETURN_STATUS_CODE HTTP_INTERNAL_SERVER_ERROR;
}

pid_t Transaction::ExecuteCgi() {
  SetCgiEnv();
  int pid = 0;
  status_code_ = cgi_.ExecuteCgi(target_resource_.c_str(),
                                 entity_.extension().c_str(), body_in_.c_str());
  if (status_code_ != HTTP_OK) {
    pid = -1;
  } else {
    pid = cgi_.pid();
  }
  FreeCgiEnv();
  return pid;
}

int Transaction::HttpGet() {
  if (!Entity::IsFileExist(target_resource_.c_str())) {
    RETURN_STATUS_CODE HTTP_NOT_FOUND;
  }
  entity_ = Entity(target_resource_);

  if (Cgi::IsSupportedCgi(entity_.extension().c_str())) {
    if (Entity::IsFileExecutable(target_resource_.c_str())) {
      cgi_.TurnOn();
      RETURN_STATUS_CODE HTTP_OK;
    } else {
      RETURN_STATUS_CODE HTTP_FORBIDDEN;
    }
  } else {
    if (Entity::IsFileReadable(target_resource_.c_str())) {
      entity_.ReadFile(target_resource_.c_str());
      SetEntityHeaders();
      RETURN_STATUS_CODE HTTP_OK;
    } else {
      RETURN_STATUS_CODE HTTP_FORBIDDEN;
    }
  }
}

int Transaction::HttpPost() {
  if (!Entity::IsFileExist(target_resource_.c_str())) {
    RETURN_STATUS_CODE HTTP_NOT_FOUND;
  }

  entity_ = Entity(target_resource_);
  if (Cgi::IsSupportedCgi(entity_.extension().c_str())) {
    if (Entity::IsFileExecutable(target_resource_.c_str())) {
      cgi_.TurnOn();
      RETURN_STATUS_CODE HTTP_OK;
    } else {
      RETURN_STATUS_CODE HTTP_FORBIDDEN;
    }
  } else {
    RETURN_STATUS_CODE HTTP_FORBIDDEN;
  }
}

int Transaction::HttpDelete() {
  if (!Entity::IsFileExist(target_resource_.c_str())) {
    RETURN_STATUS_CODE HTTP_NO_CONTENT;
  }
  if (!Entity::IsFileExecutable(target_resource_.c_str())) {
    RETURN_STATUS_CODE HTTP_FORBIDDEN;
  }

  entity_.ReadFile(target_resource_.c_str());
  SetEntityHeaders();
  if (std::remove(this->target_resource_.c_str()) != 0) {
    RETURN_STATUS_CODE HTTP_INTERNAL_SERVER_ERROR;
  }
  body_out_ = entity_.CreatePage("DELETED: " + uri_.request_target());
  RETURN_STATUS_CODE HTTP_OK;
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
        SetEnv("CONTENT_LENGTH=", headers_in_.content_length.c_str()));
  }
}

void Transaction::FreeCgiEnv() {
  typedef std::vector<char* const>::const_iterator ConstIterator;
  for (ConstIterator it = cgi_.envp().begin(); it != cgi_.envp().end(); ++it) {
    delete *it;
  }
}

std::string Transaction::AppendStatusLine() {
  return response_ += http_version_ + " " + std::to_string(status_code_) + " " +
                      HttpGetReasonPhase(status_code_) + CRLF;
}
std::string Transaction::AppendResponseHeader(const std::string key,
                                              const std::string value) {
  return response_ += key + ": " + value + CRLF;
}

std::string Transaction::CreateResponseMessage() {
  /*
        The "Date" header field represents the date and time at which the
        message was originated. Section 6.6.1 of [HTTP]
  */
  headers_out_.date_t = std::time(NULL);
  headers_out_.date = MakeRfc850Time(headers_out_.date_t);

  AppendStatusLine();

  AppendResponseHeader("Server", headers_out_.server);
  AppendResponseHeader("Date", headers_out_.date);
  static const int CLASS_OF_RESPONSE = status_code_ / 100;
  switch (CLASS_OF_RESPONSE) {
    case HTTP_INFORMATIONAL:
      break;
    case HTTP_SUCCESSFUL:
      AppendResponseHeader("Content-Type", headers_out_.content_type);
      AppendResponseHeader("Content-Length", headers_out_.content_length);
      AppendResponseHeader("Last-Modified", headers_out_.last_modified);
      break;
    case HTTP_REDIRECTION:
      AppendResponseHeader("Location", headers_out_.location);
      break;
    case HTTP_CLIENT_ERROR:
    case HTTP_SERVER_ERROR:
      // ERROR Page
      static const std::string STATUS =
          std::to_string(status_code_) + " " + HttpGetReasonPhase(status_code_);
      entity_.CreatePage(STATUS);
      SetEntityHeaders();
      AppendResponseHeader("Content-Type", headers_out_.content_type);
      AppendResponseHeader("Content-Length", headers_out_.content_length);
      break;
    default:
      break;
  }

  static const int STATUS_CODE = status_code_;
  switch (STATUS_CODE) {
    case HTTP_NOT_ALLOWED:
      AppendResponseHeader("Allow", headers_out_.allow);
      break;
    default:
      break;
  }

  response_ += CRLF;
  response_ += body_out_;
  return response_;
}
