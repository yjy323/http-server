#include "entity.hpp"

#include "utils.hpp"

Entity::Entity()
    : path_(""),
      type_(kFile),
      body_(""),
      extension_(""),
      mime_type_(""),
      modified_s_(""),
      modified_t_(0),
      length_(""),
      length_n_(0) {}

Entity::Entity(std::string path)
    : path_(path),
      type_(),
      body_(""),
      extension_(GetExtension(path)),
      mime_type_(),
      modified_s_(""),
      modified_t_(0),
      length_(""),
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
    this->modified_s_ = obj.modified_s_;
    this->modified_t_ = obj.modified_t_;
    this->length_ = obj.length_;
    this->length_n_ = obj.length_n_;
  }
  return *this;
}

std::string Entity::path() const { return this->path_; }
Entity::eType Entity::type() { return this->type_; }
std::string Entity::body() const { return this->body_; }
std::string Entity::extension() const { return this->extension_; }
std::string Entity::mime_type() const { return this->mime_type_; }
std::string Entity::modified_s() const { return this->modified_s_; }
std::time_t Entity::modified_t() const { return this->modified_t_; }
std::string Entity::length() const { return this->length_; }
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
  size_t delimiter_pos = path.rfind(".");
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

std::time_t Entity::GetModifiedTime(std::string path) {
  struct stat fileInfo;
  if (stat(path.c_str(), &fileInfo) == 0) {
    modified_t_ = fileInfo.st_mtime;
    modified_s_ = MakeRfc850Time(modified_t_);
  } else {
    modified_s_ = "";
  }
  return modified_t_;
}

std::string Entity::CreatePage(std::string body_line) {
  mime_type_ = GetMimeType("html");
  body_ =
      "<html>\n"
      "<head><title>" +
      body_line +
      "</title></head>\n"
      "<body>\n"
      "<center><h1>" +
      body_line +
      "</h1></center>\n"
      "<hr><center>nginx/1.25.4</center>\n"
      "</body>\n"
      "</html>";
  length_n_ = body_.size();
  length_ = std::to_string(length_n_);
  return body_;
}
// void CreateDirectoryListingPage();

std::string Entity::ReadFile(const char* path) {
  mime_type_ = GetMimeType(extension_);
  body_ = GetContents(path);
  modified_t_ = GetModifiedTime(path);
  length_n_ = body_.size();
  length_ = std::to_string(length_n_);
  return body_;
}

void Entity::ReadBuffer(const char* buff, size_t size) {
  (void)buff;
  (void)size;
}
