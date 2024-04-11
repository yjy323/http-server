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

#include "request.hpp"

class Cgi {
 private:
 public:
  Cgi();
  Cgi(const Cgi& obj);
  ~Cgi();
  Cgi& operator=(const Cgi& obj);

  std::vector<const char*> envp_;
  int ExecuteCgi(const char*, std::string&);
};

#endif
