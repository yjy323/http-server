#include "client.hpp"

Client::Client()
    : server_(),
      fd_(-1),
      request_(),
      response_(),
      request_str_(""),
      response_str_("") {}

Client::Client(const Server& server, int fd)
    : server_(server),
      fd_(fd),
      request_(),
      response_(),
      request_str_(""),
      response_str_("") {}

Client::Client(const Client& ref)
    : server_(ref.server()),
      fd_(ref.fd()),
      request_str_(ref.request_str()),
      response_str_(ref.response_str()) {}

Client::~Client() {}

Client& Client::operator=(const Client& ref) {
  if (this == &ref) return *this;

  this->server_ = ref.server();
  this->fd_ = ref.fd();
  this->request_str_ = ref.request_str();
  this->response_str_ = ref.response_str();

  return *this;
}

void Client::MakeResponse() {
  const Server& sk = this->server_;

  this->request_.uri_.ReconstructTargetUri(this->request_.http_host_);
  const ServerConfiguration& sc = sk.ConfByHost(this->request_.http_host_);

  this->response_ = Response(this->request_, sc);
  this->response_.HttpTransaction();
}

const int& Client::fd() const { return this->fd_; }

const Server& Client::server() const { return this->server_; }

const std::string& Client::request_str() const { return this->request_str_; }

const std::string& Client::response_str() const { return this->response_str_; }

const Request& Client::request() const { return this->request_; }

const Response& Client::response() const { return this->response_; }

Request& Client::request_instance() { return this->request_; }

Response& Client::response_instance() { return this->response_; }

void Client::set_request_str(const std::string& request_str) {
  this->request_str_ = request_str;
}

void Client::set_response_str(const std::string& response_str) {
  this->response_str_ = response_str;
}
