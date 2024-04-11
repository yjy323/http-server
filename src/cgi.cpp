#include "cgi.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

Cgi::Cgi() {}
Cgi::Cgi(const Cgi& obj) {}
Cgi::~Cgi() {}
Cgi& Cgi::operator=(const Cgi& obj) {}

int Cgi::ExecuteCgi(const char* cgi_path, std::string& extension) {
  std::ifstream ifs(cgi_path);
  if (!ifs.is_open()) {
    return 404;
  }

  std::vector<char* const> argv;
  std::vector<char* const> envp;

  if (extension == "cgi") {
    argv.push_back(const_cast<char*>(cgi_path));
    argv.push_back(NULL);

  } else if (extension == "py") {
    std::string buffer;
    std::getline(ifs, buffer);
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

  const char* request_method = "REQUEST_METHOD=GET";
  const char* content_length = "CONTENT_LENGTH=15";
  const char* query_string = "QUERY_STRING=username=jooyoung";

  envp.push_back(const_cast<char*>(request_method));
  envp.push_back(const_cast<char*>(content_length));
  envp.push_back(const_cast<char*>(query_string));

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

    execve(*argv.data(), argv.data(), envp.data());
    std::exit(500);
  } else {
    int orig_stdin = dup(STDIN_FILENO);
    int orig_stdout = dup(STDOUT_FILENO);

    close(server2cgi_fd[0]);
    dup2(server2cgi_fd[1], STDOUT_FILENO);
    close(server2cgi_fd[1]);

    // write(STDOUT_FILENO, "username=beachu", 15);

    close(cgi2server_fd[1]);
    dup2(cgi2server_fd[0], STDIN_FILENO);
    close(cgi2server_fd[0]);

    char buffer[4096];
    std::string response;

    ssize_t count;
    while ((count = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
      response.append(buffer, count);
    }

    int status;
    waitpid(pid, &status, 0);

    dup2(orig_stdin, STDIN_FILENO);
    dup2(orig_stdout, STDOUT_FILENO);
    close(orig_stdin);
    close(orig_stdout);

    // 파이프로부터 응답을 읽어옴
    std::cout << response.substr(0, response.find("\r\n\r\n") + 4);
    std::cout << response.substr(response.find("\r\n\r\n") + 4);
  }

  return 0;
}
