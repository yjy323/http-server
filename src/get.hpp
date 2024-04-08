#ifndef GET_HPP
#define GET_HPP

/*
        기능 구현을 위한 임시 클래스
        TODO: HTTP Transaction의 하위 메서드로 변경
*/

#include "request.hpp"

class Get {
 private:
  Request Request;

 public:
  void GetStaticFile(std::string& resource);
  void GetCgiScript(std::string& resource);
  void GetDirectory(std::string& resource);
  void GetDirectoryListing(std::string& resource);
};

#endif
