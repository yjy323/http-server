#include "response.hpp"

Response::Response() : request_(), server_conf_() {}
Response::Response(const Request& request,
                   const ServerConfiguration& server_conf)
    : request_(request),
      server_conf_(server_conf),
      response_header_(""),
      response_body_(""),
      response_message_(""),
      status_line_("") {}
Response::Response(const Response& obj) { (void)obj; }
Response::~Response() {}
Response& Response::operator=(const Response& obj) {
  if (this == &obj) return *this;

  request_ = obj.request_;
  server_conf_ = obj.server_conf_;
  loc_conf_ = obj.loc_conf_;
  request_target_ = obj.request_target_;
  target_resource_ = obj.target_resource_;
  target_resource_type_ = obj.target_resource_type_;
  response_message_ = obj.response_message_;

  return *this;
}

void Response::FindResourceConfiguration() {
  size_t most_specific_pos = 0;
  LocConfIterator begin = server_conf_.location().begin();
  LocConfIterator end = server_conf_.location().end();
  std::string request_target = request_.uri_.request_target_;

  for (LocConfIterator it = begin; it != end; ++it) {
    std::string key = it->first;
    size_t pos = request_target.find(key);
    if (pos == request_target.npos && most_specific_pos != 0) {
      break;
    } else if (pos == 0 && most_specific_pos < key.length()) {
      loc_conf_ = it->second;
      most_specific_pos = key.length();
    }
  }

  if (most_specific_pos == 0) {
    ServerConfiguration sc = server_conf_;
    std::set<std::string> default_allowed_method;
    default_allowed_method.insert("GET");

    loc_conf_ = LocationConfiguration(
        sc.error_page(), sc.client_max_body_size(), sc.root(), sc.auto_index(),
        sc.index(), default_allowed_method, "", "");
  }
}

bool Response::IsAllowedMethod(const char* method) {
  std::set<std::string> allowed_method = loc_conf_.allowed_method();
  if (allowed_method.find(method) != allowed_method.end()) {
    return true;
  } else {
    return false;
  }
}

int Response::HttpTransaction() {
  FindResourceConfiguration();
  SetTargetResource();

  // TODO: 메소드 명 define
  std::string method = request_.method_;
  if (IsAllowedMethod(method.c_str())) {
    if (method == HTTP_GET_METHOD) {
      return HttpGetMethod();
    } else if (method == HTTP_POST_METHOD) {
      return HttpPostMethod();
    } else if (method == HTTP_DELETE_METHOD) {
      return HttpDeleteMethod();
    }
  }
  return HTTP_NOT_ALLOWED;
}

void Response::SetTargetResource() {
  request_target_ = request_.uri_.request_target_;
  if (request_target_ == loc_conf_.return_uri()) {
    // redirect URI
  }
  target_resource_ = "." + loc_conf_.root() + request_.uri_.request_target_;

  if (target_resource_[target_resource_.length() - 1] == '/') {
    target_resource_type_ = kDirectory;
    target_resource_extension_ = "";

  } else {
    target_resource_type_ = kFile;

    target_resource_extension_ =
        target_resource_.substr(target_resource_.rfind('.'));
    if (target_resource_extension_.length() > 0) {
      target_resource_extension_ = target_resource_extension_.erase(0, 1);
    }
  }
}

void Response::SetResponseMessage() {
  status_line_ = "HTTP/1.1 " + std::to_string(status_code_);
  std::time_t datetime = std::time(NULL);
  response_header_ = "Server: webserv 1.0" CRLF;
  "Date: " + MakeRfc850Time(datetime) + CRLF + response_header_;

  if (status_code_ == HTTP_OK) {
  } else if (status_code_ == HTTP_OK) {
  } else if (status_code_ == HTTP_OK) {
  } else if (status_code_ == HTTP_OK) {
  } else if (status_code_ == HTTP_OK) {
  } else {
  }
}

int Response::GetMimeType() {
  response_header_ += "Content-Type: ";
  if (target_resource_extension_ == "html") {
    response_header_ += "text/html" CRLF;
  } else if (target_resource_extension_ == "css") {
    response_header_ += "text/css" CRLF;
  } else {
    response_header_ += "text/plain" CRLF;
  }
  return HTTP_OK;
}

int Response::GetStaticFile() {
  if (access(target_resource_.c_str(), F_OK) == -1) {
    return HTTP_NOT_FOUND;
  } else if (access(target_resource_.c_str(), R_OK) == -1) {
    return HTTP_FORBIDDEN;
  }

  std::ifstream file(target_resource_);

  std::ostringstream oss;
  oss << file.rdbuf();
  response_body_ = oss.str();

  struct stat fileInfo;
  std::time_t modified_time;
  if (stat(target_resource_.c_str(), &fileInfo) == 0) {
    modified_time = fileInfo.st_mtime;
  }
  response_header_ += "Last-Modified: " + MakeRfc850Time(modified_time) + CRLF;
  GetMimeType();
  response_header_ +=
      "Content-Length: " + std::to_string(response_body_.size()) + CRLF;

  return HTTP_OK;
}

int Response::GetCgiScript() {
  if (access(target_resource_.c_str(), F_OK) == -1) {
    return HTTP_NOT_FOUND;
  } else if (access(target_resource_.c_str(), X_OK | W_OK | R_OK) == -1) {
    return HTTP_FORBIDDEN;
  }

  std::ifstream file(target_resource_);
  struct stat fileInfo;
  std::time_t modified_time;
  if (stat(target_resource_.c_str(), &fileInfo) == 0) {
    modified_time = fileInfo.st_mtime;
  }
  response_header_ += "Last-Modified: " + MakeRfc850Time(modified_time) + CRLF;
  Cgi::ExecuteCgi(target_resource_.c_str(), target_resource_extension_, *this);
  return HTTP_OK;
}

int Response::HttpGetMethod() {
  if (target_resource_type_ == kDirectory) {
    // index
    // autoindex
    return HTTP_FORBIDDEN;
  } else if (Cgi::IsSupportedCgi(target_resource_extension_)) {
    GetCgiScript();
    SetResponseHeader();
    return HTTP_OK;
  } else {
    GetStaticFile();
    SetResponseHeader();
    return HTTP_OK;
  }
}

int Response::HttpPostMethod() {
  if (target_resource_type_ == kDirectory) {
    return HTTP_FORBIDDEN;
  } else if (Cgi::IsSupportedCgi(target_resource_extension_)) {
    GetCgiScript();
    return HTTP_OK;
  } else {
    return HTTP_FORBIDDEN;
  }
}

int Response::HttpDeleteMethod() {
  if (access(target_resource_.c_str(), F_OK) == -1) {
    return HTTP_NO_CONTENT;
  } else if (access(target_resource_.c_str(), W_OK | X_OK) == -1) {
    return HTTP_FORBIDDEN;
  } else {
    std::remove(target_resource_.c_str());
    return HTTP_OK;
  }
}
