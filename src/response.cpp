#include "response.hpp"

int Response::HttpTransaction() {
  std::string method = this->request_.method_;
  if (method == "GET") {
    HttpGetMethod();
  } else if (method == "POST") {
    HttpPostMethod();
  } else if (method == "DELETE") {
    HttpDeleteMethod();
  } else {
    return 405;
  }
}

int Response::HttpGetMethod() {}
int Response::HttpPostMethod() {}
int Response::HttpDeleteMethod() {}
