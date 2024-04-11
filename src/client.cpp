#include "client.hpp"

Client::Client() : server_(), fd_(-1), request_str_(""), response_("") {}

Client::Client(const Server& server, int fd)
    : server_(server), fd_(fd), request_str_(""), response_("") {}

Client::Client(const Client& ref)
    : server_(ref.server()),
      fd_(ref.fd()),
      request_str_(ref.request_str()),
      response_(ref.response()) {}

Client::~Client() {}

Client& Client::operator=(const Client& ref) {
  if (this == &ref) return *this;

  this->server_ = ref.server();
  this->fd_ = ref.fd();
  this->request_str_ = ref.request_str();
  this->response_ = ref.response();

  return *this;
}

const int& Client::fd() const { return this->fd_; }

const Server& Client::server() const { return this->server_; }

const std::string& Client::request_str() const { return this->request_str_; }

const std::string& Client::response() const { return this->response_; }

const Request& Client::request() const { return this->request_; }

Request& Client::request_instance() { return this->request_; }

void Client::set_request_str(const std::string& request) {
  this->request_str_ = request;
}

void Client::set_response(const std::string& response) {
  this->response_ = response;
}
