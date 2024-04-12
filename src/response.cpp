#include "response.hpp"

Response::Response(Request& request, ServerConfiguration& server_conf)
    : request_(request), server_conf_(server_conf) {}
Response::Response(const Response& obj) { (void)obj; }
Response::~Response() {}
Response& Response::operator=(const Response& obj) {
  (void)obj;
  return *this;
}

void Response::FindResourceConfiguration() {
  size_t most_specific_pos = 0;
  LocConfIterator begin = this->server_conf_.location().begin();
  LocConfIterator end = this->server_conf_.location().end();
  std::string request_target = this->request_.uri_.request_target_;

  for (LocConfIterator it = begin; it != end; ++it) {
    std::string key = it->first;
    size_t pos = request_target.find(key);
    if (pos == request_target.npos && most_specific_pos != 0) {
      break;
    } else if (pos == 0 && most_specific_pos < key.length()) {
      this->loc_conf_ = it->second;
      most_specific_pos = key.length();
    }
  }

  if (most_specific_pos == 0) {
    ServerConfiguration sc = this->server_conf_;
    std::set<std::string> default_allowed_method;
    default_allowed_method.insert("GET");

    this->loc_conf_ = LocationConfiguration(
        sc.error_page(), sc.client_max_body_size(), sc.root(), sc.auto_index(),
        sc.index(), default_allowed_method, "", "");
  }
}

bool Response::IsAllowedMethod(const char* method) {
  std::set<std::string> allowed_method = this->loc_conf_.allowed_method();
  if (allowed_method.find(method) != allowed_method.end()) {
    return true;
  } else {
    return false;
  }
}

int Response::HttpTransaction() {
  this->FindResourceConfiguration();
  this->response_header_ = "";
  this->response_body_ = "";
  this->response_message_ = "";
  this->status_line_ = "";

  request_target_ = this->request_.uri_.request_target_;
  if (request_target_ == this->loc_conf_.return_uri()) {
    // redirect URI
  }
  this->target_resource_ =
      "." + this->loc_conf_.root() + this->request_.uri_.request_target_;

  if (target_resource_[target_resource_.length() - 1] == '/') {
    this->target_resource_type_ = kDirectory;
    this->target_resource_extension_ = "";

  } else {
    this->target_resource_type_ = kFile;

    this->target_resource_extension_ =
        this->target_resource_.substr(this->target_resource_.rfind('.'));
    if (target_resource_extension_.length() > 0) {
      target_resource_extension_ = target_resource_extension_.erase(0, 1);
    }
  }
  // TODO: 메소드 명 define
  std::string method = this->request_.method_;
  if (method == "GET\0" && IsAllowedMethod("GET\0")) {
    HttpGetMethod();
  } else if (method == "POST\0" && IsAllowedMethod("POST\0")) {
    HttpPostMethod();
  } else if (method == "DELETE\0" && IsAllowedMethod("DELETE\0")) {
    HttpDeleteMethod();
  } else {
    return 405;
  }

  SetStatusLine();
  this->response_message_ = this->status_line_ + this->response_header_ +
                            "\r\n" + this->response_body_;
  return OK;
}

void Response::SetStatusLine() { this->status_line_ = "HTTP/1.1 200 OK\r\n"; }

int Response::SetResponseHeader() {
  // Server
  // Date
  std::time_t datetime = std::time(NULL);
  this->response_header_ =
      "Server: Webserv 1.0\r\n"
      "Date: " +
      MakeRfc850Time(datetime) + "\r\n" + this->response_header_;
  return OK;
}

int Response::GetMimeType() {
  this->response_header_ += "Content-Type: ";
  if (this->target_resource_extension_ == "html") {
    this->response_header_ += "text/html\r\n";
  } else if (this->target_resource_extension_ == "css") {
    this->response_header_ += "style/css\r\n";
  } else {
    this->response_header_ += "text/plain\r\n";
  }
  return OK;
}

int Response::GetStaticFile() {
  if (access(this->target_resource_.c_str(), F_OK) == -1) {
    return 404;
  } else if (access(this->target_resource_.c_str(), R_OK) == -1) {
    return 403;
  }

  std::ifstream file(this->target_resource_);

  std::ostringstream oss;
  oss << file.rdbuf();
  this->response_body_ = oss.str();

  struct stat fileInfo;
  std::time_t modified_time;
  if (stat(target_resource_.c_str(), &fileInfo) == 0) {
    modified_time = fileInfo.st_mtime;
  }
  this->response_header_ +=
      "Last-Modified: " + MakeRfc850Time(modified_time) + "\r\n";
  GetMimeType();
  this->response_header_ +=
      "Content-Length: " + std::to_string(this->response_body_.size()) + "\r\n";

  return 200;
}

int Response::GetCgiScript() {
  if (access(this->target_resource_.c_str(), F_OK) == -1) {
    return 404;
  } else if (access(this->target_resource_.c_str(), X_OK | W_OK | R_OK) == -1) {
    return 403;
  }

  std::ifstream file(this->target_resource_);
  struct stat fileInfo;
  std::time_t modified_time;
  if (stat(target_resource_.c_str(), &fileInfo) == 0) {
    modified_time = fileInfo.st_mtime;
  }
  this->response_header_ +=
      "Last-Modified: " + MakeRfc850Time(modified_time) + "\r\n";
  Cgi::ExecuteCgi(this->target_resource_.c_str(),
                  this->target_resource_extension_, *this);
  return OK;
}

int Response::HttpGetMethod() {
  if (this->target_resource_type_ == kDirectory) {
    // index
    // autoindex
    return 403;
  } else if (Cgi::IsSupportedCgi(this->target_resource_extension_)) {
    GetCgiScript();
    SetResponseHeader();
    return OK;
  } else {
    GetStaticFile();
    SetResponseHeader();
    return OK;
  }
}

int Response::HttpPostMethod() {
  if (this->target_resource_type_ == kDirectory) {
    return 403;
  } else if (Cgi::IsSupportedCgi(target_resource_extension_)) {
    GetCgiScript();
    return OK;
  } else {
    return 403;
  }
}

int Response::HttpDeleteMethod() {
  if (access(this->target_resource_.c_str(), F_OK) == -1) {
    return 204;
  } else if (access(this->target_resource_.c_str(), W_OK | X_OK) == -1) {
    return 403;
  } else {
    std::remove(this->target_resource_.c_str());
    return OK;
  }
}
