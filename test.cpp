#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string generateMultipartFormData() {
  // 임의의 구분자 생성
  std::string boundary = "----Boundary123456789";

  // 요청 본문 생성
  std::ostringstream requestBody;
  requestBody << "--" << boundary << "\r\n";
  requestBody << "Content-Disposition: form-data; name=\"file\"; "
                 "filename=\"test.txt\"\r\n\r\n";
  requestBody << "user\r\n";
  requestBody << "--" << boundary << "\r\n";
  requestBody << "Content-Disposition: form-data; name=\"file2\"; "
                 "filename=\"test2.txt\"\r\n\r\n";
  requestBody << "password\r\n";
  requestBody << "--" << boundary << "--\r\n";

  return requestBody.str();
}

int main() {
  int cgi2server_fd_[2];
  int server2cgi_fd_[2];
  pid_t pid_;

  std::string form_data = generateMultipartFormData();
  std::cout << form_data;

  std::string request_method = "REQUEST_METHOD=POST";
  std::string content_length =
      "CONTENT_LENGTH=" + std::to_string(form_data.length());
  std::string content_type =
      "CONTENT_TYPE=multipart/form-data; "
      "boundary=----Boundary123456789";

  char* argv_[] = {"/usr/bin/python3", "./www/cgi-bin/upload.py", NULL};
  char* envp_[] = {const_cast<char*>(request_method.c_str()),
                   const_cast<char*>(content_type.c_str()),
                   const_cast<char*>(content_length.c_str()), NULL};

  if (pipe(cgi2server_fd_) == -1 || pipe(server2cgi_fd_) == -1) {
    return 1;
  }

  pid_ = fork();
  if (pid_ == -1) {
    return 1;

  } else if (pid_ == 0) {
    close(server2cgi_fd_[1]);
    dup2(server2cgi_fd_[0], STDIN_FILENO);
    close(server2cgi_fd_[0]);

    close(cgi2server_fd_[0]);
    dup2(cgi2server_fd_[1], STDOUT_FILENO);
    close(cgi2server_fd_[1]);

    execve(argv_[0], argv_, envp_);
    exit(1);
  } else {
    close(server2cgi_fd_[0]);
    close(cgi2server_fd_[1]);

    write(server2cgi_fd_[1], const_cast<char*>(form_data.c_str()),
          form_data.length());
    close(server2cgi_fd_[1]);

    char buffer[4096];
    std::string response;

    ssize_t count;
    while ((count = read(cgi2server_fd_[0], buffer, sizeof(buffer))) > 0) {
      response.append(buffer, count);
    }
    close(cgi2server_fd_[0]);

    int status;
    waitpid(pid_, &status, 0);

    // 파이프로부터 응답을 읽어옴
    std::cout << response;
  }

  return 0;
}
