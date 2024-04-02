#ifndef URL_HPP
#define URL_HPP

#include <cctype>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "abnf.hpp"
#include "utils.hpp"

#define MAX_URI_LENGTH 8000  // Todo: 최대 길이 정의

class Url {
 public:
  enum RequestTargetFrom {
    kOriginForm,
    kAbsoluteForm,
    kAuthorityForm,
    kAsteriskForm
  };

  struct PathSegment {
    std::string path;
    std::map<const std::string, std::string> parameter;
  };

 private:
  RequestTargetFrom request_target_form_;
  /*
        URI Component의 구성요소
  */

  std::string scheme_;
  std::string user_;
  std::string password_;
  std::string host_;
  int port_;  // todo - socket API의 port 자료구조로 변경
  std::list<PathSegment> path_segments_;  // todo - 자료구조 결정 필요
  std::map<const std::string, std::string> query_;

  int ParseScheme(std::string& scheme);
  int ParseAuthority(std::string& authority);
  int ParsePathSegment(std::string& path_component);
  int ParseQuery(std::string& query);

 public:
  Url();
  ~Url();
  Url(const Url& obj);
  Url& operator=(const Url& obj);

  int ParseUriComponent(std::string& request_uri);
};

#endif
