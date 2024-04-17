#include "cgi.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

Cgi::Cgi()
    : argv_(std::vector<char* const>()),
      envp_(std::vector<char* const>()),
      cgi2server_fd_(),
      server2cgi_fd_(),
      pid_(0),
      on_(false) {}

Cgi::Cgi(const Cgi& obj) { *this = obj; }
Cgi::~Cgi() {}
Cgi& Cgi::operator=(const Cgi& obj) {
  if (this != &obj) {
    // this->argv_ = obj.argv_;
    // this->envp_ = obj.envp_;
    this->cgi2server_fd_[0] = obj.cgi2server_fd_[0];
    this->cgi2server_fd_[1] = obj.cgi2server_fd_[1];
    this->server2cgi_fd_[0] = obj.server2cgi_fd_[0];
    this->server2cgi_fd_[1] = obj.server2cgi_fd_[1];
    this->pid_ = obj.pid_;
    this->on_ = obj.on_;
  }
  return *this;
}

const std::vector<char* const>& Cgi::argv() const { return this->argv_; }
const std::vector<char* const>& Cgi::envp() const { return this->envp_; }
std::vector<char* const>& Cgi::argv() { return this->argv_; }
std::vector<char* const>& Cgi::envp() { return this->envp_; }

const int* Cgi::cgi2server_fd() const { return this->cgi2server_fd_; }
const int* Cgi::server2cgi_fd() const { return this->server2cgi_fd_; }
pid_t Cgi::pid() const { return this->pid_; }
bool Cgi::on() const { return this->on_; }

bool Cgi::TurnOn() {
  on_ = true;
  return on_;
}

bool Cgi::IsSupportedCgi(const char* extension) {
  if (std::strncmp(extension, CGI_FILE, std::strlen(CGI_FILE) + 1) ||
      std::strncmp(extension, CGI_FILE, std::strlen(PY_FILE) + 1)) {
    return true;
  } else {
    return false;
  }
}

bool Cgi::IsCgiProgram(const char* extension) {
  if (std::strncmp(extension, CGI_FILE, std::strlen(CGI_FILE) + 1)) {
    return true;
  } else {
    return false;
  }
}

bool Cgi::IsCgiScript(const char* extension) {
  if (std::strncmp(extension, CGI_FILE, std::strlen(PY_FILE) + 1)) {
    return true;
  } else {
    return false;
  }
}

pid_t Cgi::ExecuteCgi(const char* path, const char* extension,
                      const char* form_data) {
  std::ifstream ifs(path);

  if (IsCgiProgram(extension)) {
    argv_.push_back(const_cast<char*>(path));
    argv_.push_back(NULL);

  } else if (IsCgiScript(extension)) {
    std::string buffer;
    std::getline(ifs, buffer, '\n');
    if (buffer.size() < 2 || buffer.find("#!") != 0) {
      return HTTP_FORBIDDEN;
    }

    argv_.push_back(const_cast<char*>(buffer.replace(0, 2, "").c_str()));
    argv_.push_back(const_cast<char*>(path));
    argv_.push_back(NULL);
  }

  if (pipe(cgi2server_fd_) == -1 || pipe(server2cgi_fd_) == -1) {
    return HTTP_INTERNAL_SERVER_ERROR;
  }

  pid_ = fork();
  if (pid_ == -1) {
    return HTTP_INTERNAL_SERVER_ERROR;

  } else if (pid_ == 0) {
    close(server2cgi_fd_[1]);
    dup2(server2cgi_fd_[0], STDIN_FILENO);
    close(server2cgi_fd_[0]);

    close(cgi2server_fd_[0]);
    dup2(cgi2server_fd_[1], STDOUT_FILENO);
    close(cgi2server_fd_[1]);

    execve(*argv_.data(), argv_.data(), envp_.data());
    exit(1);
  } else {
    close(server2cgi_fd_[0]);
    close(cgi2server_fd_[1]);
    write(server2cgi_fd_[1], form_data, std::strlen(form_data));
    close(server2cgi_fd_[1]);
  }

  return HTTP_OK;
}
