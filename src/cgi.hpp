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

struct CgiHeaders {
  std::string content_type;
  std::string location;
  std::string status;

  int status_code;
};
void CgiContentType(CgiHeaders&, const std::string);
void CgiLocation(CgiHeaders&, const std::string);
void CgiStatus(CgiHeaders&, const std::string);

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
  std::string response() const;
  CgiHeaders headers() const;
  CgiHeaders& headers_instance();

  void set_response(std::string response);
  void set_pid(pid_t pid);
  void set_server2cgi_fd(int idx, int server2cgi_fd);

 private:
  bool IsCgiProgram(const char*);
  bool IsCgiScript(const char*);

  std::vector<char*> argv_;
  std::vector<char*> envp_;
  int cgi2server_fd_[2];
  int server2cgi_fd_[2];
  pid_t pid_;
  bool on_;

  std::string response_;
  CgiHeaders headers_;
};

#endif
