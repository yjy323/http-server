#include "client.hpp"

Client::Client()
    : server_(), fd_(-1), transaction_(), request_str_(""), response_str_("") {}

Client::Client(const Server& server, int fd)
    : server_(server),
      fd_(fd),
      transaction_(),
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

const int& Client::fd() const { return this->fd_; }

const Server& Client::server() const { return this->server_; }

const std::string& Client::request_str() const { return this->request_str_; }

const std::string& Client::response_str() const { return this->response_str_; }

const Transaction& Client::transaction() const { return this->transaction_; }

Transaction& Client::transaction_instance() { return this->transaction_; }

void Client::set_request_str(const std::string& request_str) {
  this->request_str_ = request_str;
}

void Client::set_response_str(const std::string& response_str) {
  this->response_str_ = response_str;
}
