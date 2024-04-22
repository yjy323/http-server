#include "cgi.hpp"

#define CGI_FILE ".cgi\0"
#define PY_FILE ".py\0"

typedef std::vector<char* const>::const_iterator Iterator;

/*
 Cgi 비멤버 함수
*/
void CgiContentType(CgiHeaders& headers, const std::string value) {
  headers.content_type = value;
}

void CgiLocation(CgiHeaders& headers, const std::string value) {
  headers.location = value;
}

void CgiStatus(CgiHeaders& headers, const std::string value) {
  char* end_ptr = NULL;
  headers.status = value;
  std::string status_code = value.substr(value.find(' '));
  long status_code_n = std::strtol(status_code.c_str(), &end_ptr, 10);
  if (end_ptr == value || *end_ptr != 0 ||
      !(status_code_n >= 100 && status_code_n <= 600)) {
    headers.status_code = HTTP_BAD_GATEWAY;
  } else {
    headers.status_code = status_code_n;
  }
}

Cgi::Cgi()
    : argv_(std::vector<char*>()),
      envp_(std::vector<char*>()),
      cgi2server_fd_(),
      server2cgi_fd_(),
      pid_(0),
      on_(false),
      response_(""),
      headers_() {
  headers_.status_code = HTTP_OK;
}

Cgi::Cgi(const Cgi& obj) { *this = obj; }
Cgi::~Cgi() {
  for (Iterator it = envp_.begin(); it != envp_.end(); ++it) {
    delete *it;
  }
}
Cgi& Cgi::operator=(const Cgi& obj) {
  if (this != &obj) {
    for (Iterator it = obj.argv_.begin(); it != obj.argv_.end(); ++it) {
      this->argv_.push_back(*it);
    }
    for (Iterator it = obj.envp_.begin(); it != obj.envp_.end(); ++it) {
      char* env = new char[std::strlen(*it) + 1];
      std::strcpy(env, *it);
      this->envp_.push_back(env);
    }
    this->cgi2server_fd_[0] = obj.cgi2server_fd_[0];
    this->cgi2server_fd_[1] = obj.cgi2server_fd_[1];
    this->server2cgi_fd_[0] = obj.server2cgi_fd_[0];
    this->server2cgi_fd_[1] = obj.server2cgi_fd_[1];
    this->pid_ = obj.pid_;
    this->on_ = obj.on_;
    this->response_ = obj.response_;
    this->headers_ = obj.headers_;
  }
  return *this;
}

void Cgi::set_response(std::string response) { this->response_ = response; }

const std::vector<char*>& Cgi::argv() const { return this->argv_; }
const std::vector<char*>& Cgi::envp() const { return this->envp_; }
std::vector<char*>& Cgi::argv() { return this->argv_; }
std::vector<char*>& Cgi::envp() { return this->envp_; }
const int* Cgi::cgi2server_fd() const { return this->cgi2server_fd_; }
const int* Cgi::server2cgi_fd() const { return this->server2cgi_fd_; }
pid_t Cgi::pid() const { return this->pid_; }
bool Cgi::on() const { return this->on_; }
std::string Cgi::response() const { return this->response_; }
CgiHeaders Cgi::headers() const { return this->headers_; }

CgiHeaders& Cgi::headers_instance() { return this->headers_; }

bool Cgi::TurnOn() {
  on_ = true;
  return on_;
}

bool Cgi::IsSupportedCgi(const char* extension) {
  if (std::strncmp(extension, CGI_FILE, std::strlen(CGI_FILE) + 1) == 0 ||
      std::strncmp(extension, PY_FILE, std::strlen(PY_FILE) + 1) == 0) {
    return true;
  } else {
    return false;
  }
}

bool Cgi::IsCgiProgram(const char* extension) {
  if (std::strncmp(extension, CGI_FILE, std::strlen(CGI_FILE) + 1) == 0) {
    return true;
  } else {
    return false;
  }
}

bool Cgi::IsCgiScript(const char* extension) {
  if (std::strncmp(extension, PY_FILE, std::strlen(PY_FILE) + 1)) {
    return true;
  } else {
    return false;
  }
}

pid_t Cgi::ExecuteCgi(const char* path, const char* extension,
                      const char* form_data) {
  if (IsCgiProgram(extension)) {
    argv_.push_back(const_cast<char*>(path));
    argv_.push_back(NULL);

  } else if (IsCgiScript(extension)) {
    std::ifstream ifs(path);
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
    // while()
    write(server2cgi_fd_[1], form_data, std::strlen(form_data));
    close(server2cgi_fd_[1]);
  }

  return HTTP_OK;
}
