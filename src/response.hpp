#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "request.hpp"
#include "utils.hpp"

class Response {
 private:
  Request request_;

 public:
  Response();
  Response(const Response& obj);
  ~Response();
  Response& operator=(const Response& obj);

  int HttpTransaction();
  int HttpGetMethod();
  int HttpPostMethod();
  int HttpDeleteMethod();
};

#endif
