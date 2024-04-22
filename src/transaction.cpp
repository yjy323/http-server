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
std::string SetPath(std::string root, std::string path);

char* SetEnv(const char* key, const char* value) {
  char* env = new char[std::strlen(key) + std::strlen(value) + 1];
  std::strcpy(env, key);
  std::strcat(env, value);
  return env;
}

std::string SetRootPath(std::string root, std::string path) {
  std::string root_path;
  if (root.length() > 0 && root[0] != '.') {
    root_path.append(".");
  }
  root_path.append(root);
  if (path.length() > 0 && path[0] != '/') {
    root_path.append("/");
  }
  root_path.append(path);
  return root_path;
}
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
      cgi_(),
      response_("") {
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
    this->response_ = obj.response_;
    this->cgi_ = obj.cgi_;
  }
  return *this;
}

int Transaction::ParseRequestHeader(std::string buff) {
  /*
          HTTP-message = start-line CRLF
                        *( field-line CRLF )
                        CRLF
                        [ message-body ]
  */
  const ssize_t SIZE = buff.size();
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
      if (ParseRequestLine(line) != HTTP_OK) {
        RETURN_STATUS_CODE status_code_;
      } else {
        start_line_flag = false;
        continue;
      }
    } else if (line.empty()) {
      if (headers_in_.host == "") {
        RETURN_STATUS_CODE HTTP_BAD_REQUEST;
      } else {
        RETURN_STATUS_CODE HTTP_OK;
      }
    } else {
      if (ParseFieldValue(line) != HTTP_OK) {
        RETURN_STATUS_CODE status_code_;
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

int Transaction::HttpProcess() {
  if (config_.return_uri() != "") {
    headers_out_.location = "/" + config_.return_uri();
    RETURN_STATUS_CODE HTTP_MOVED_PERMANENTLY;
  }

  target_resource_ = SetRootPath(config_.root(), uri_.decoded_request_target());

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
    RETURN_STATUS_CODE HttpDelete();
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
  return pid;
}

std::string Transaction::CreateResponseMessage() {
  /*
        The "Date" header field represents the date and time at which the
        message was originated. Section 6.6.1 of [HTTP]
  */

  response_ = std::string();

  if (status_code_ == HTTP_OK && cgi_.on()) {
    SetResponseFromCgi();
  }

  headers_out_.connection_close = headers_in_.connection_close;
  headers_out_.date_t = std::time(NULL);
  headers_out_.date = MakeRfc850Time(headers_out_.date_t);

  if (body_out_ == "") {
    entity_.CreatePage(ToString(status_code_) + " " +
                       HttpGetReasonPhase(status_code_));
    SetResponseFromEntity();
  }

  AppendResponseHeader("Server", headers_out_.server);
  AppendResponseHeader("Date", headers_out_.date);
  const int CLASS_OF_RESPONSE = status_code_ / 100;
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

  const int STATUS_CODE = status_code_;
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

  AppendStatusLine();
  response_ += CRLF;
  if (body_out_ == "") {
  } else {
    response_ += body_out_;
  }
  return response_;
}

/*
        Getter
*/
const Transaction::Configuration& Transaction::config() const {
  return this->config_;
}
std::string Transaction::method() const { return this->method_; }
const Uri& Transaction::uri() const { return this->uri_; }
const HeadersIn& Transaction::headers_in() const { return this->headers_in_; }
std::string Transaction::body_in() const { return this->body_in_; }
std::string Transaction::http_version() const { return this->http_version_; }
std::string Transaction::server_version() const {
  return this->server_version_;
}
int Transaction::status_code() const { return this->status_code_; }
std::string Transaction::target_resource() const {
  return this->target_resource_;
}
const HeadersOut& Transaction::headers_out() const {
  return this->headers_out_;
}
std::string Transaction::body_out() const { return this->body_out_; }
const Entity& Transaction::entity() const { return this->entity_; }
Entity& Transaction::entity() { return this->entity_; }
const Cgi& Transaction::cgi() const { return this->cgi_; }
Cgi& Transaction::cgi() { return this->cgi_; }
std::string Transaction::response() const { return this->response_; }

Uri& Transaction::uri_instance() { return this->uri_; }
Cgi& Transaction::cgi_instance() { return this->cgi_; }

/*
        Setter
*/
void Transaction::set_status_code(const int& status_code) {
  this->status_code_ = status_code;
}

/*
        Private methods
*/
int Transaction::ParseRequestLine(std::string& request_line) {
  std::vector<std::string> request_line_component = Split(request_line, ' ');
  if (request_line_component.size() != 3) {
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }
  std::string method = request_line_component[0];
  std::string request_target = request_line_component[1];
  std::string http_version = request_line_component[2];
  if (method.length() == 0 || !IsToken(method, "")) {
    RETURN_STATUS_CODE HTTP_BAD_REQUEST;
  }
  if ((status_code_ = this->uri_.ParseUriComponent(request_target)) !=
      HTTP_OK) {
    RETURN_STATUS_CODE status_code_;
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
  if (field_name.length() == 0 || !IsToken(field_name, "")) {
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
    IF_BAD_HEADER_THEN_RETURN(HttpContentType(headers_in_, field_value));
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

int Transaction::DecodeChunkedEncoding(std::string buff) {
  const size_t SIZE = buff.size();
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
    if (contents.length() != (size_t)std::strtol(hex_str.c_str(), NULL, 16)) {
      RETURN_STATUS_CODE HTTP_BAD_REQUEST;
    }

    body_in_.append(contents);

    offset = contents_end + std::strlen(CRLF);
  }
  headers_in_.content_length_n = body_in_.length();
  headers_in_.content_length = ToString(headers_in_.content_length_n);
  RETURN_STATUS_CODE HTTP_OK;
}

int Transaction::HttpGet() {
  if (!Entity::IsFileExist(target_resource_.c_str())) {
    RETURN_STATUS_CODE HTTP_NOT_FOUND;
  }
  entity_ = Entity(target_resource_);
  if (entity_.type() == Entity::kDirectory) {
    std::string index_file = SetRootPath(config_.root(), config_.index());
    if (config_.index() != "" && Entity::IsFileReadable(index_file.c_str())) {
      target_resource_ = index_file;
      RETURN_STATUS_CODE HttpGet();
    } else if (config_.auto_index()) {
      entity_.CreateDirectoryListingPage(target_resource_.c_str(),
                                         uri_.decoded_request_target().c_str());
      SetResponseFromEntity();
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
      SetResponseFromEntity();
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

  if (std::remove(this->target_resource_.c_str()) != 0) {
    RETURN_STATUS_CODE HTTP_INTERNAL_SERVER_ERROR;
  }
  RETURN_STATUS_CODE HTTP_OK;
}

void Transaction::SetCgiEnv() {
  cgi_.envp().push_back(SetEnv("AUTH_TYPE=", ""));
  cgi_.envp().push_back(SetEnv("GATEWAY_INTERFACE=", ""));
  cgi_.envp().push_back(SetEnv("PATH_TRANSLATED=", ""));
  cgi_.envp().push_back(SetEnv("REMOTE_ADDR=", ""));
  cgi_.envp().push_back(SetEnv("REMOTE_HOST=", ""));
  cgi_.envp().push_back(SetEnv("REMOTE_IDENT=", ""));
  cgi_.envp().push_back(SetEnv("SCRIPT_NAME=", ""));
  cgi_.envp().push_back(SetEnv("SERVER_NAME=", ""));
  cgi_.envp().push_back(SetEnv("SERVER_PORT=", ""));
  cgi_.envp().push_back(SetEnv("SERVER_PROTOCOL=", "HTTP/1.1"));
  cgi_.envp().push_back(SetEnv("SERVER_SOFTWARE=", "webserv/1.0"));

  if (method_ == HTTP_GET_METHOD) {
    cgi_.envp().push_back(SetEnv("REQUEST_METHOD=", "GET"));
    cgi_.envp().push_back(SetEnv("QUERY_STRING=", uri_.query_string().c_str()));

  } else if (method_ == HTTP_POST_METHOD) {
    std::string path_info = SetRootPath(config_.root(), config_.upload_store());
    cgi_.envp().push_back(SetEnv("REQUEST_METHOD=", "POST"));
    cgi_.envp().push_back(
        SetEnv("CONTENT_LENGTH=", headers_in_.content_length.c_str()));
    cgi_.envp().push_back(
        SetEnv("CONTENT_TYPE=", headers_in_.content_type.c_str()));

    cgi_.envp().push_back(SetEnv("PATH_INFO=", path_info.c_str()));
  }
  cgi_.envp().push_back(NULL);
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

void Transaction::SetErrorPage() {
  const std::string ERROR_PAGE_PATH =
      SetRootPath(config_.root(), config_.error_page());

  if (config_.error_page() != "" &&
      entity_.IsFileReadable(ERROR_PAGE_PATH.c_str())) {
    entity_ = Entity(ERROR_PAGE_PATH);
    entity_.ReadFile(ERROR_PAGE_PATH.c_str());
    SetResponseFromEntity();
  }
}

void Transaction::SetResponseFromEntity() {
  body_out_ = entity_.body();
  headers_out_.content_type = entity_.mime_type();
  headers_out_.content_length = entity_.length();
  headers_out_.last_modified = entity_.modified_s();
}

void Transaction::SetResponseFromCgi() {
  typedef std::vector<std::string>::const_iterator Iterator;
  const std::string DUPLICATED_NL = "\n\n";
  // const std::string DUPLICATED_CRLF = "\r\n\r\n";  // For CGI_Tester
  std::string header;
  std::string body;
  size_t pos = 0;

  pos = cgi_.response().find(DUPLICATED_NL);
  if (pos == cgi_.response().npos) {
    status_code_ = HTTP_BAD_GATEWAY;
  } else {
    header = cgi_.response().substr(0, pos);
    body = cgi_.response().substr(pos + DUPLICATED_NL.length());
    std::vector<std::string> headers = Split(header, '\n');
    for (Iterator it = headers.begin(); it != headers.end(); ++it) {
      pos = (*it).find(':');
      if (pos == (*it).npos) {
        continue;
      }
      std::string field = ToCaseInsensitive((*it).substr(0, pos));
      std::string value = Trim((*it).substr(pos + 1));
      if (field == "content-type") {
        CgiContentType(cgi_.headers_instance(), value);
      } else if (field == "location") {
        CgiLocation(cgi_.headers_instance(), value);
      } else if (field == "status") {
        CgiStatus(cgi_.headers_instance(), value);
      }
    }

    headers_out_.content_type = cgi_.headers().content_type;
    headers_out_.location = cgi_.headers().location;
    status_code_ = cgi_.headers().status_code;
    body_out_ = body;
    headers_out_.content_length_n = body_out_.length();
    headers_out_.content_length = ToString(headers_out_.content_length_n);
  }
}

std::string Transaction::AppendStatusLine() {
  return response_ = http_version_ + " " + ToString(status_code_) + " " +
                     HttpGetReasonPhase(status_code_) + CRLF + response_;
}

std::string Transaction::AppendResponseHeader(const std::string key,
                                              const std::string value) {
  if (value == "") {
    return "";
  }
  return response_ += key + ": " + value + CRLF;
}
