#include "cgi.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

Cgi::Cgi() {}
Cgi::Cgi(const Cgi& obj) {}
Cgi::~Cgi() {}
Cgi& Cgi::operator=(const Cgi& obj) {}

void SetEnvironment() {
  /*
                REQUEST_METHOD(GET, POST)
                QUERY_STRING(GET)
                CONTENT_TYPE(POST)
                CONTENT_LENGTH(POST)
  */
}

int Cgi::ExecuteScript(const char* script_path) {
  std::ifstream file(script_path);
  if (!file.is_open()) {
    return 404;
  }
  std::string buffer;
  std::getline(file, buffer);
  buffer.replace(buffer.find("#!"), 2, "");
  const char* cgi_script = buffer.c_str();
  const char* request_method = "REQUEST_METHOD=POST";
  const char* content_length = "CONTENT_LENGTH=15";
  const char* query_string = "QUERY_STRING=username=jooyoung";
  char* const argv[] = {const_cast<char*>(cgi_script),
                        const_cast<char*>(script_path), NULL};

  char* const envp[] = {const_cast<char*>(request_method),
                        const_cast<char*>(content_length),
                        const_cast<char*>(query_string), NULL};

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
    close(cgi2server_fd[1]);

    execve(cgi_script, argv, envp);
  } else {
    int orig_stdin = dup(STDIN_FILENO);
    int orig_stdout = dup(STDOUT_FILENO);

    close(server2cgi_fd[0]);
    dup2(server2cgi_fd[1], STDOUT_FILENO);
    close(server2cgi_fd[1]);

    write(STDOUT_FILENO, "username=beachu", 15);

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
    close(server2cgi_fd[1]);
    close(cgi2server_fd[0]);
    close(orig_stdin);
    close(orig_stdout);

    // 파이프로부터 응답을 읽어옴

    std::cout << response;
  }

  return 0;
}
