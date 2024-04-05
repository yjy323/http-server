#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>

int main() {
  // CGI 스크립트 경로 및 인자 설정
  const char* cgi_script = "/usr/bin/python3";
  const char* script_path =
      "/Users/jy_23/Drive/42Seoul/repo/webserv/webserv/cgi-bin/welcome.py";
  const char* username = "배추";
  char* const argv[] = {const_cast<char*>(cgi_script),
                        const_cast<char*>(script_path),
                        const_cast<char*>("REQUEST_METHOD=POST"),
                        const_cast<char*>("username=배추"), NULL};

  // 환경 변수 설정
  char* const envp[] = {NULL};

  // 파이프 생성
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    std::cerr << "Failed to create pipe" << std::endl;
    return 1;
  }

  // 새로운 프로세스 생성
  pid_t pid = fork();

  if (pid == -1) {
    // fork 실패
    std::cerr << "Failed to fork" << std::endl;
    return 1;
  } else if (pid == 0) {
    // 자식 프로세스
    // 표준 출력을 파이프로 리다이렉션
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    // CGI 스크립트 실행
    execve(cgi_script, argv, envp);

    // execve가 성공하지 않으면 아래 코드가 실행됩니다.
    std::cerr << "Failed to execute CGI script" << std::endl;
    return 1;
  } else {
    // 부모 프로세스
    close(pipefd[1]);
    char buffer[4096];
    std::string response;

    // 파이프로부터 응답을 읽어옴
    ssize_t count;
    while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
      response.append(buffer, count);
    }

    // 파이프 닫기
    close(pipefd[0]);

    // 자식 프로세스가 종료될 때까지 대기
    int status;
    waitpid(pid, &status, 0);

    // 응답 출력
    std::cout << "Received response from CGI script:\n"
              << response << std::endl;
  }

  return 0;
}
