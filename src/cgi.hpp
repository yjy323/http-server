#ifndef CGI_HPP
#define CGI_HPP

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "response.hpp"

class Response;

class Cgi {
 private:
 public:
  static bool IsSupportedCgi(std::string&);
  static int ExecuteCgi(const char* cgi_path, const std::string& extension,
                        Response& response);
};

#endif
