#include "cgi.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

char* SetEnv(const char*, const char*);

char* SetEnv(const char* key, const char* value) {
  char* env = new char[std::strlen(key) + std::strlen(value) + 1];
  std::strcpy(env, key);
  std::strcat(env, value);
  return (char* const)env;
}

bool Cgi::IsSupportedCgi(std::string& extension) {
  if (extension == "cgi" || extension == "py") {
    return true;
  } else {
    return false;
  }
}

int Cgi::ExecuteCgi(const char* cgi_path, const std::string& extension,
                    Response& response) {
  std::ifstream ifs(cgi_path);

  std::vector<char* const> argv;

  if (extension == "cgi\0") {
    argv.push_back(const_cast<char*>(cgi_path));
    argv.push_back(NULL);

  } else if (extension == "py\0") {
    std::string buffer;
    std::getline(ifs, buffer, '\n');
    if (buffer.size() > 2 && buffer.find("#!") == 0) {
      buffer.replace(0, 2, "");
    } else {
      return 403;
    }

    const char* cgi_program = buffer.c_str();
    argv.push_back(const_cast<char*>(cgi_program));
    argv.push_back(const_cast<char*>(cgi_path));
    argv.push_back(NULL);

  } else {
    return 403;
  }
  int cgi2server_fd[2];
  int server2cgi_fd[2];

  if (pipe(cgi2server_fd) == -1 || pipe(server2cgi_fd) == -1) {
    return 500;
  }

  pid_t pid = fork();
  if (pid == -1) {
    return 500;

  } else if (pid == 0) {
    close(server2cgi_fd[1]);
    dup2(server2cgi_fd[0], STDIN_FILENO);
    close(server2cgi_fd[0]);

    close(cgi2server_fd[0]);
    dup2(cgi2server_fd[1], STDOUT_FILENO);
    close(cgi2server_fd[1]);

    std::vector<char* const> envp;
    if (response.request_.method_ == "GET\0") {
      envp.push_back(SetEnv("REQUEST_METHOD=", "GET"));
      envp.push_back(SetEnv("QUERY_STRING=",
                            response.request_.uri_.query_string_.c_str()));

    } else if (response.request_.method_ == "POST\0") {
      envp.push_back(SetEnv("REQUEST_METHOD=", "POST"));
      envp.push_back(SetEnv(
          "CONTENT_LENGTH=",
          std::to_string(response.request_.http_content_length_).c_str()));
    }

    execve(*argv.data(), argv.data(), envp.data());
    // ERROR
    std::exit(500);
  } else {
    int orig_stdin = dup(STDIN_FILENO);
    int orig_stdout = dup(STDOUT_FILENO);

    close(server2cgi_fd[0]);
    dup2(server2cgi_fd[1], STDOUT_FILENO);
    close(server2cgi_fd[1]);

    write(STDOUT_FILENO, response.request_.body_.c_str(),
          response.request_.http_content_length_);

    close(cgi2server_fd[1]);
    dup2(cgi2server_fd[0], STDIN_FILENO);
    close(cgi2server_fd[0]);

    char buffer[8192];
    std::string cgi_response;

    ssize_t count;
    while ((count = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
      cgi_response.append(buffer, count);
    }

    int status;
    waitpid(pid, &status, 0);

    dup2(orig_stdin, STDIN_FILENO);
    dup2(orig_stdout, STDOUT_FILENO);
    close(orig_stdin);
    close(orig_stdout);

    response.response_header_ +=
        cgi_response.substr(0, cgi_response.find("\r\n\r\n") + 4);
    response.response_body_ +=
        cgi_response.substr(cgi_response.find("\r\n\r\n") + 4);
  }

  return 0;
}
