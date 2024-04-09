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

  request_target_ = this->request_.uri_.request_target_;
  if (request_target_ == this->loc_conf_.return_uri()) {
    // redirect URI
  }
  target_resource_ =
      "." + this->loc_conf_.root() + this->request_.uri_.request_target_;

  if (target_resource_[target_resource_.length() - 1] == '/') {
    this->target_resource_type_ = kDirectory;
  } else {
    this->target_resource_type_ = kFile;
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
  return OK;
}

std::string ReadFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return "File not found.";
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  return oss.str();
}

int Response::HttpGetMethod() {
  if (this->target_resource_type_ == kDirectory) {
    // index
    // autoindex
    // 403
  } else {
    // CGI
    std::string extension =
        this->target_resource_.substr(this->target_resource_.rfind('.'));
    if (extension.length() > 0) {
      extension = extension.erase(0, 1);
    }

    if (extension == ".py") {
    } else {
      // Server
      // Content-Length
      // Content-Type
      // Date
      // Last-Modified

      std::time_t datetime = std::time(NULL);
      struct stat fileInfo;

      std::time_t modified_time;
      if (stat(target_resource_.c_str(), &fileInfo) == 0) {
        modified_time = fileInfo.st_mtime;
      }

      std::string status_line = "HTTP/1.1 200 OK\r\n";
      std::string content = ReadFile(target_resource_);
      status_line += "Server: Webserv\r\n";
      status_line +=
          "Content-Length: " + std::to_string(content.length()) + "\r\n";
      status_line += "Content-Type: text/" + extension + "\r\n";
      status_line += "Date: " + MakeRfc850Time(datetime) + "\r\n";
      status_line += "Last-Modified: " + MakeRfc850Time(modified_time) + "\r\n";
      status_line += "\r\n" + content;

      response_message_ = status_line;
    }
    // 404
  }
  return OK;
}
int Response::HttpPostMethod() {
  if (this->target_resource_type_ == kDirectory) {
    // Upload File
    // 403
  } else {
    // CGI
    // 404
  }
  return OK;
}
int Response::HttpDeleteMethod() {
  // DELETE
  // 200 201
  return OK;
}
