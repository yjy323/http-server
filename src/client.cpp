#include "client.hpp"

Client::Client() : server_(), fd_(-1), request_(""), response_("") {}

Client::Client(const Server& server, int fd)
    : server_(server), fd_(fd), request_(""), response_("") {}

Client::Client(const Client& ref)
    : server_(ref.server()),
      fd_(ref.fd()),
      request_(ref.request()),
      response_(ref.response()) {}

Client::~Client() {}

Client& Client::operator=(const Client& ref) {
  if (this == &ref) return *this;

  this->server_ = ref.server();
  this->fd_ = ref.fd();
  this->request_ = ref.request();
  this->response_ = ref.response();

  return *this;
}

const int& Client::fd() const { return this->fd_; }

const Server& Client::server() const { return this->server_; }

const std::string& Client::request() const { return this->request_; }

const std::string& Client::response() const { return this->response_; }

void Client::set_request(const std::string& request) {
  this->request_ = request;
}

void Client::set_response(const std::string& response) {
  this->response_ = response;
}
