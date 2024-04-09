#ifndef CGI_HPP
#define CGI_HPP

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Cgi {
 private:
 public:
  Cgi();
  Cgi(const Cgi& obj);
  ~Cgi();
  Cgi& operator=(const Cgi& obj);

  int ExecuteScript(const char*);
};

#endif
