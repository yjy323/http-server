#ifndef URL_HPP
#define URL_HPP

#include <iostream>
#include <list>
#include <map>

/*
        1. request uri 파싱
        2. request 유효성 검사 ? 에러 코드 반환 방법
        3. 리소스 정보 획득 ?
        4. static 클래스 여부 결정 ?
*/

class Url {
 private:
  enum RequestTargetFrom {
    kOriginForm,
    kAbsoluteForm,
    kAuthorityForm,
    kAsteriskForm
  };

  /*
        todo - ResourceType 타입과 변수의 선언 위치 결정 필요
   */
  enum ResourceType { kFile, kDirectory, kGateway };
  ResourceType resource_type_;

  struct PathComponent {
    std::string path;
    std::map<std::string, std::string> parameter;
  };

  /*
        URI Component의 구성요소
  */
  std::string scheme_;
  std::string user_;
  std::string password_;
  std::string host_;
  int port_;  // todo - socket API의 port 자료구조로 변경
  std::list<PathComponent> path_component_;  // todo - 자료구조 결정 필요
  std::map<std::string, std::string> query_string_;

  std::string target_uri_;
  RequestTargetFrom request_target_form_;
  // todo - 서버 정보를 저장할 변수 필요
  int ParseScheme(std::string& scheme);
  int ParseAuthority(std::string& authority);
  int ParsePathComponent(std::string& path_component);

 public:
  Url();
  Url(const Url& obj);
  ~Url();
  Url& operator=(const Url& obj);
  int ParseUriComponent(std::string& request_uri);
  // todo - Impl method, URI 컴포넌트의 값을 채운다.
  void ReconstructTargetUri();
  /*
        todo - Impl method
        파싱된 컴포넌트 정보를 이용해 target URI를 생성한다.
        response에 쓰이지는 않을 것 같음
  */
};

#endif
