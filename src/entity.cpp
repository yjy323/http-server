#include "entity.hpp"

#include <ctime>

#include "utils.hpp"

#define HTML_FILE ".html\0"
#define CSS_FILE ".css\0"

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
      type_(GetType(path)),
      body_(""),
      extension_(GetExtension(path)),
      mime_type_(GetMimeType(extension_)),
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
  std::ifstream file;
  file.open(path.c_str());
  std::ostringstream oss;
  oss << file.rdbuf();
  body_ = oss.str();
  return body_;
}

Entity::eType Entity::GetType(std::string path) {
  if (path.size() != 0 && path[path.size() - 1] == '/') {
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
    extension_ = path.substr(delimiter_pos);
  }
  return extension_;
}

std::string Entity::GetMimeType(std::string extension) {
  if (extension == HTML_FILE) {
    mime_type_ = "text/html";
  } else if (extension == CSS_FILE) {
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
  body_ =
      "<html>\n"
      "<head><title>" +
      body_line +
      "</title></head>\n"
      "<body>\n"
      "<center><h1>" +
      body_line +
      "</h1></center>\n"
      "<hr><center>webserv</center>\n"
      "</body>\n"
      "</html>";
  mime_type_ = GetMimeType(HTML_FILE);
  length_n_ = body_.size();
  length_ = ToString(length_n_);
  return body_;
}
std::string Entity::CreateDirectoryListingPage(std::string path,
                                               std::string request_target) {
  std::stringstream html;
  html << "<!DOCTYPE html>\n";
  html << "<html>\n";
  html << "<head><title>Index of " << request_target << "</title></head>\n";
  html << "<body>\n";
  html << "<h1>Index of " << request_target << "</h1><hr><pre>\n";

  html << "<a href=\"../\">../</a>\n";

  DIR* dir;
  struct dirent* entry;
  struct stat fileStat;
  char buffer[80];

  if ((dir = opendir(path.c_str())) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      std::string filename = entry->d_name;
      if (filename != "." && filename != "..") {
        std::string filepath = path + "/" + filename;
        if (stat(filepath.c_str(), &fileStat) == 0) {
          struct tm* timeinfo = std::localtime(&fileStat.st_mtime);
          std::strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", timeinfo);
          if (S_ISDIR(fileStat.st_mode)) {
            filename += "/";
          }
          html << "<a href=\"" << filename << "\">" << filename << "</a>\t\t";
          html << std::right << std::setw(10) << buffer << "\t";
          if (S_ISDIR(fileStat.st_mode)) {
            html << std::setw(10) << "----\t";
          } else {
            html << std::setw(10) << fileStat.st_size << "\t";
          }
          html << "\n";
        }
      }
    }
    closedir(dir);
  } else {
    html << "<p>Error opening directory</p>\n";
  }

  html << "</pre><hr></body>\n";
  html << "</html>\n";

  body_ = html.str();
  mime_type_ = GetMimeType(HTML_FILE);
  length_n_ = body_.size();
  length_ = ToString(length_n_);
  return body_;
}

std::string Entity::ReadFile(const char* path) {
  body_ = GetContents(path);
  modified_t_ = GetModifiedTime(path);
  length_n_ = body_.size();
  length_ = ToString(length_n_);
  return body_;
}
