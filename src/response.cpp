#include "response.hpp"

Response::Response() : request_() {}
Response::Response(const Request& request)
    : request_(request),
      config_(Configuration()),
      status_code_(HTTP_OK),
      headers_(HeadersOut()),
      body_(""),
      target_resource_(""),
      target_resource_extension_(""),
      target_resource_type_(kFile) {}

Response::Response(const Response& obj) { *this = obj; }

Response::~Response() {}

Response& Response::operator=(const Response& obj) {
  if (this != &obj) {
    this->request_ = obj.request_;
    this->config_ = obj.config_;
    this->status_code_ = obj.status_code_;
    this->headers_ = obj.headers_;
    this->body_ = obj.body_;
    this->target_resource_ = obj.target_resource_;
    this->target_resource_extension_ = obj.target_resource_extension_;
    this->target_resource_type_ = obj.target_resource_type_;
  }
  return *this;
}

void Response::SetConfiguration(const ServerConfiguration& server_config) {
  typedef std::map<std::string, Configuration>::const_iterator
      ConfigurationIterator;

  size_t max_match_pos = 0;

  for (ConfigurationIterator it = server_config.location().begin();
       it != server_config.location().end(); ++it) {
    std::string key = it->first;
    size_t match_pos = request_.uri().request_target().find(key);
    if (match_pos == request_.uri().request_target().npos &&
        max_match_pos != 0) {
      break;
    } else if (match_pos == 0 && max_match_pos < key.length()) {
      config_ = it->second;
      max_match_pos = key.length();
    }
  }

  if (max_match_pos == 0) {
    std::set<std::string> default_allowed_method;
    default_allowed_method.insert("GET");

    config_ = Response::Configuration(
        server_config.error_page(), server_config.client_max_body_size(),
        server_config.root(), server_config.auto_index(), server_config.index(),
        default_allowed_method, "", "");
  }
}

void Response::SetTargetResource() {
  target_resource_ = "." + config_.root() + request_.uri().request_target();

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

bool Response::IsRedirectedUri() {
  std::string request_target = request_.uri().request_target();
  if (request_.uri().request_target() == config_.return_uri()) {
    // redirect URI
  }
}

bool Response::IsAllowedMethod(const char* method) {
  if (config_.allowed_method().find(method) != config_.allowed_method().end()) {
    return true;
  } else {
    return false;
  }
}

std::string GetMimeType(std::string extension) {
  if (extension == "html") {
    return "text/html";
  } else if (extension == "css") {
    return "text/css";
  } else {
    return "text/plain";
  }
}

std::string GetContents(std::string path) {
  std::ifstream file(path);
  std::ostringstream oss;
  oss << file.rdbuf();
  return oss.str();
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
