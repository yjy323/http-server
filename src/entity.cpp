#include "entity.hpp"

Entity::Entity()
    : path_(""),
      type_(kFile),
      body_(""),
      extension_(""),
      mime_type_(""),
      length_n_(0) {}

Entity::Entity(std::string path)
    : path_(path),
      type_(),
      body_(""),
      extension_(GetExtension(path)),
      mime_type_(),
      length_n_(0) {}

Entity::Entity(const Entity& obj) { *this = obj; }
Entity::~Entity() {}
Entity& Entity::operator=(const Entity& obj) {
  if (this != &obj) {
    this->path_ = obj.path_;
    this->type_ = obj.type_;
    this->body_ = obj.body_;
    this->extension_ = obj.extension_;
    this->mime_type_ = obj.mime_type_;
    this->length_n_ = obj.length_n_;
  }
  return *this;
}

std::string Entity::path() const { return this->path_; }
Entity::eType Entity::type() { return this->type_; }
std::string Entity::body() const { return this->body_; }
std::string Entity::extension() const { return this->extension_; }
std::string Entity::mime_type() const { return this->mime_type_; }
ssize_t Entity::length_n() { return this->length_n_; }

bool Entity::IsFileExist(const char* path) {
  if (access(path, F_OK) == 0) {
    return true;
  } else {
    return false;
  }
}

bool Entity::IsFileReadable(const char* path) {
  if (access(path, R_OK) == 0) {
    return true;
  } else {
    return false;
  }
}

bool Entity::IsFileExecutable(const char* path) {
  if (access(path, X_OK | R_OK | W_OK) == 0) {
    return true;
  } else {
    return false;
  }
}

std::string Entity::GetContents(std::string path) {
  std::ifstream file(path);
  std::ostringstream oss;
  oss << file.rdbuf();
  body_ = oss.str();
  return body_;
}

Entity::eType Entity::GetType(std::string path) {
  if (path.size() != 0 && *path.end() == '/') {
    return kDirectory;
  } else {
    return kFile;
  }
}
std::string Entity::GetExtension(std::string path) {
  size_t delimiter_pos = path.find(".");
  if (delimiter_pos == path.npos) {
    extension_ = "";
  } else {
    extension_ = path.substr(delimiter_pos + 1);
  }
  return extension_;
}

std::string Entity::GetMimeType(std::string extension) {
  if (extension == "html") {
    mime_type_ = "text/html";
  } else if (extension == "css") {
    mime_type_ = "text/css";
  } else {
    mime_type_ = "text/plain";
  }
  return mime_type_;
}

void Entity::ReadFile() {
  mime_type_ = GetMimeType(extension_);
  body_ = GetContents(path_);
  length_n_ = body_.size();
}

void Entity::ReadBuffer() {}
