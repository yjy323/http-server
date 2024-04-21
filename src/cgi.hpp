#ifndef CGI_HPP
#define CGI_HPP

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "http.hpp"

class Response;

class Cgi {
 public:
  Cgi();
  ~Cgi();
  Cgi(const Cgi& obj);
  Cgi& operator=(const Cgi& obj);

  static bool IsSupportedCgi(const char*);
  int ExecuteCgi(const char*, const char*, const char*);
  bool TurnOn();

  const std::vector<char*>& argv() const;
  const std::vector<char*>& envp() const;
  std::vector<char*>& argv();
  std::vector<char*>& envp();
  const int* cgi2server_fd() const;
  const int* server2cgi_fd() const;
  pid_t pid() const;
  bool on() const;

 private:
  bool IsCgiProgram(const char*);
  bool IsCgiScript(const char*);

  std::vector<char*> argv_;
  std::vector<char*> envp_;
  int cgi2server_fd_[2];
  int server2cgi_fd_[2];
  pid_t pid_;
  bool on_;
};

#endif
