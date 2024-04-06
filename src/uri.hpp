#ifndef URI_HPP
#define URI_HPP

#include <cctype>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "abnf.hpp"
#include "configuration.hpp"
#include "utils.hpp"

#define MAX_URI_LENGTH 8000  // Todo: 최대 길이 정의

class Uri {
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

  Uri();
  Uri(const Uri& obj);
  ~Uri();
  Uri& operator=(const Uri& obj);

  int ParseUriComponent(std::string& request_uri);
  int ReconstructTargetUri(std::string& request_host);

  int ParseScheme(std::string& scheme);
  int ParseAuthority(std::string& authority);
  int ParsePathSegment(std::string& path_component);
  int ParseQuery(std::string& query);

  RequestTargetFrom request_target_form_;
  std::string request_target_;

  std::string scheme_;
  std::string user_;
  std::string password_;
  std::string host_;
  int port_;  // todo - socket API의 port 자료구조로 변경
  std::list<PathSegment> path_segments_;
  std::map<const std::string, std::string> query_;
};

#endif
