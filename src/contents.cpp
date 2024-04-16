#include "contents.hpp"

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
