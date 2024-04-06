#include "get.hpp"

#include <sys/stat.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string read_file(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return "File not found.";
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  return oss.str();
}

void Get::GetStaticFile(std::string& resource) {
  // Server
  // Content-Length
  // Content-Type
  // Date
  // Last-Modified

  std::time_t datetime = std::time(NULL);
  struct stat fileInfo;

  std::time_t modified_time;
  if (stat(resource.c_str(), &fileInfo) == 0) {
    modified_time = fileInfo.st_mtime;
  }

  std::string status_line = "HTTP/1.1 200 OK\r\n";
  std::string content = read_file(resource);
  status_line += "Server: Webserv\r\n";
  status_line += "Content-Length: " + std::to_string(content.length()) + "\r\n";
  status_line += "Content-Type: text/html\r\n";
  status_line += "Date: " + MakeRfc850Time(datetime) + "\r\n";
  status_line += "Last-Modified: " + MakeRfc850Time(modified_time) + "\r\n";
  status_line += "\r\n" + read_file(resource);

  std::cout << status_line;
}

void Get::GetCgiScript(std::string& script_path) {
  std::time_t datetime = std::time(NULL);
  struct stat fileInfo;

  std::string extension = script_path.substr(script_path.rfind("."));
  if (extension == ".py") {
    const char* cgi_script = "/usr/bin/python3";
  }

  char* const envp[] = {NULL};
}
// void Get::GetDirectory(std::string& resource) {}
// void Get::GetDirectoryListing(std::string& resource) {}
