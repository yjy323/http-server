#include "client.hpp"

Client::Client() : fd_(-1), request_(""), response_("") {}

Client::Client(int fd) : fd_(fd), request_(""), response_("") {}

Client::Client(const Client& ref)
    : fd_(ref.fd()), request_(ref.request()), response_(ref.response()) {}

Client::~Client() {}

Client& Client::operator=(const Client& ref) {
  if (this == &ref) return *this;

  this->fd_ = ref.fd();
  this->request_ = ref.request();
  this->response_ = ref.response();

  return *this;
}

const int& Client::fd() const { return this->fd_; }

const std::string& Client::request() const { return this->request_; }

const std::string& Client::response() const { return this->response_; }

void Client::set_request(const std::string& request) {
  this->request_ = request;
}

void Client::set_response(const std::string& response) {
  this->response_ = response;
}
