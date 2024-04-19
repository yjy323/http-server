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
  headers_out_.connection = HTTP_CONNECTION_OPTION_KEEP_ALIVE;
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
Uri& Transaction::uri() { return this->uri_; }
const Entity& Transaction::entity() const { return this->entity_; }
Entity& Transaction::entity() { return this->entity_; }
const Cgi& Transaction::cgi() const { return this->cgi_; }
const HeadersIn& Transaction::headers_in() const { return this->headers_in_; }
const std::string& Transaction::body() const { return this->body_in_; }
const Transaction::Configuration& Transaction::config() const {
  return this->config_;
}
const int& Transaction::status_code() const { return this->status_code_; }

void Transaction::set_status_code(const int& status_code) {
  this->status_code_ = status_code;
}

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
  if (method.length() == 0 || !IsToken(method, false)) {
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
  if (field_name.length() == 0 || !IsToken(field_name, false)) {
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

int Transaction::ParseRequestHeader(std::string buff) {
  /*
          HTTP-message = start-line CRLF
                        *( field-line CRLF )
                        CRLF
                        [ message-body ]
  */
  static const ssize_t SIZE = buff.size();
  ssize_t offset = 0;
  std::string line;
  size_t crlf_pos;

  bool start_line_flag = true;

  while (offset < SIZE) {
    crlf_pos = buff.find(CRLF);
    if (crlf_pos == buff.npos) {
      // CRLF가 존재하지 않는 요청 해더
      // return status_code_ = HTTP_BAD_REQUEST;
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
    }

    line = buff.substr(0, crlf_pos);
    buff = buff.substr(crlf_pos + 2);
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

int Transaction::ParseRequestBody(std::string buff, size_t content_length) {
  std::stringstream ss;
  size_t offset = 0;

  if (headers_in_.chuncked) {
    RETURN_STATUS_CODE DecodeChunkedEncoding(buff);
  } else {
    for (; offset < content_length; ++offset) {
      char c = buff[offset];
      if (!IsOctet(c)) {
        RETURN_STATUS_CODE HTTP_BAD_REQUEST;
      }
      ss << c;
    }
    this->body_in_ = ss.str();
    RETURN_STATUS_CODE HTTP_OK;
  }
}

int Transaction::DecodeChunkedEncoding(std::string buff) {
  static const size_t SIZE = buff.size();
  size_t offset = 0;

  while (offset < SIZE) {
    if (buff.substr(offset, std::strlen("0" CRLF)) == "0" CRLF) {
      break;
    }

    std::size_t hex_end = buff.find(CRLF, offset);
    if (hex_end == std::string::npos) {
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
    }

    std::string hex_str = buff.substr(offset, hex_end - offset);
    if (hex_str.empty()) {
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
    }
    for (size_t i = 0; i < hex_str.length(); ++i) {
      if (!std::isxdigit(hex_str[i])) {
        RETURN_STATUS_CODE HTTP_BAD_REQUEST;
      }
    }

    size_t contents_end = buff.find(CRLF, hex_end + std::strlen(CRLF));
    if (contents_end == std::string::npos) {
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
    }

    std::string contents =
        buff.substr(hex_end + std::strlen(CRLF),
                    contents_end - hex_end - std::strlen(CRLF));
    if (contents.length() != (size_t)strtol(hex_str.c_str(), NULL, 16)) {
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
    }

    body_in_.append(contents);

    offset = contents_end + std::strlen(CRLF);
  }
  headers_in_.content_length_n = body_in_.length();
  headers_in_.content_length = std::to_string(headers_in_.content_length_n);
  RETURN_STATUS_CODE HTTP_OK;
}

const Transaction::Configuration& Transaction::GetConfiguration(
    const ServerConfiguration& server_config) {
  typedef std::map<std::string, Configuration>::const_iterator
      ConfigurationIterator;
  std::string request_target = uri_.decoded_request_target();
  size_t request_target_len = request_target.length();
  size_t max_common_length = 0;

  for (ConfigurationIterator it = server_config.location().begin();
       it != server_config.location().end(); ++it) {
    std::string key = it->first;
    if (request_target.find(key) != request_target.npos) {
      size_t common_length = 0;
      size_t min_length = std::min(key.length(), request_target_len);
      for (size_t i = 0; i < min_length; ++i) {
        if (key[i] == request_target[i]) {
          common_length++;
        } else {
          break;
        }
      }
      if (common_length > max_common_length) {
        config_ = it->second;
        max_common_length = common_length;
      }
    }
  }

  if (max_common_length == 0) {
    std::set<std::string> default_allowed_method;
    default_allowed_method.insert("GET");

    config_ = Transaction::Configuration(
        server_config.error_page(), server_config.client_max_body_size(),
        server_config.root(), server_config.auto_index(), server_config.index(),
        default_allowed_method, "", "");
  }
  return config_;
}

void Transaction::SetAllowdMethod() {
  typedef std::set<std::string>::const_iterator METHOD_ITER;
  size_t method_cnt = config_.allowed_method().size();
  METHOD_ITER method = config_.allowed_method().begin();
  headers_out_.allow = "";
  for (size_t i = 0; i < method_cnt; ++i) {
    headers_out_.allow += *(method++) + (i < method_cnt - 1 ? ", " : "");
  }
}

void Transaction::SetEntityHeaders() {
  body_out_ = entity_.body();
  headers_out_.content_type = entity_.mime_type();
  headers_out_.content_length = entity_.length();
  headers_out_.last_modified = entity_.modified_s();
}

int Transaction::HttpGet() {
  if (!Entity::IsFileExist(target_resource_.c_str())) {
    RETURN_STATUS_CODE HTTP_NOT_FOUND;
  }
  entity_ = Entity(target_resource_);
  if (entity_.type() == Entity::kDirectory) {
    if (config_.index() != "") {
      target_resource_ = "." + config_.root() + "/" + config_.index();
      RETURN_STATUS_CODE HttpGet();
    } else if (config_.auto_index()) {
      entity_.CreateDirectoryListingPage(target_resource_.c_str(),
                                         uri_.decoded_request_target().c_str());
      SetEntityHeaders();
      RETURN_STATUS_CODE HTTP_OK;
    } else {
      RETURN_STATUS_CODE HTTP_FORBIDDEN;
    }
  } else if (Cgi::IsSupportedCgi(entity_.extension().c_str())) {
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
  if (entity_.type() == Entity::kDirectory) {
    RETURN_STATUS_CODE HTTP_FORBIDDEN;
  } else if (Cgi::IsSupportedCgi(entity_.extension().c_str()) &&
             Entity::IsFileExecutable(target_resource_.c_str())) {
    cgi_.TurnOn();
    RETURN_STATUS_CODE HTTP_OK;
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
  body_out_ = entity_.CreatePage("DELETED: " + uri_.decoded_request_target());
  RETURN_STATUS_CODE HTTP_OK;
}

char* SetEnv(const char* key, const char* value) {
  char* env = new char[std::strlen(key) + std::strlen(value) + 1];
  std::strcpy(env, key);
  std::strcat(env, value);
  return (char* const)env;
}

int Transaction::HttpProcess() {
  if (config_.return_uri() != "") {
    headers_out_.location = "/" + config_.return_uri();
    RETURN_STATUS_CODE HTTP_MOVED_PERMANENTLY;
  }

  target_resource_ = "." + config_.root() + uri_.decoded_request_target();

  if (config_.allowed_method().find(method_) ==
      config_.allowed_method().end()) {
    SetAllowdMethod();
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

std::string Transaction::AppendStatusLine() {
  return response_ += http_version_ + " " + std::to_string(status_code_) + " " +
                      HttpGetReasonPhase(status_code_) + CRLF;
}
std::string Transaction::AppendResponseHeader(const std::string key,
                                              const std::string value) {
  if (value == "") {
    return "";
  }
  return response_ += key + ": " + value + CRLF;
}

void Transaction::SetErrorPage() {
  static const std::string ERROR_PAGE_PATH =
      "." + config_.root() + "/" + config_.error_page();

  if (config_.error_page() != "" &&
      entity_.IsFileReadable(ERROR_PAGE_PATH.c_str())) {
    entity_ = Entity(ERROR_PAGE_PATH);
    entity_.ReadFile(ERROR_PAGE_PATH.c_str());
  } else {
    if (config_.error_page() != "") {
      status_code_ = HTTP_FORBIDDEN;
    }
    entity_.CreatePage(std::to_string(status_code_) + " " +
                       HttpGetReasonPhase(status_code_));
  }
  SetEntityHeaders();
}

std::string Transaction::CreateResponseMessage() {
  /*
        The "Date" header field represents the date and time at which the
        message was originated. Section 6.6.1 of [HTTP]
  */
  headers_out_.connection_close = headers_in_.connection_close;
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
      SetErrorPage();
      AppendResponseHeader("Content-Type", headers_out_.content_type);
      AppendResponseHeader("Content-Length", headers_out_.content_length);
      headers_out_.connection_close = true;
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

  if (headers_out_.connection_close == true) {
    headers_out_.connection = HTTP_CONNECTION_OPTION_CLOSE;
  }
  AppendResponseHeader("Connection", headers_out_.connection);
  response_ += CRLF;
  response_ += body_out_;
  return response_;
}
