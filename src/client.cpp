#include "client.hpp"

#define REQUEST_HEADER_END CRLF CRLF
#define CHUNKED_REQUEST_BODY_END 0 CRLF CRLF

static const int BUFFER_SIZE = 2048;

Client::Client(const Server& server, int fd)
    : Socket(fd, server.info()),
      server_configs_(server.config()),
      transaction_(),
      request_(),
      response_(),
      request_body_parsed_(false) {}

Client::~Client() {}

Client& Client::operator=(const Client& ref) {
  if (&ref == this) return *this;

  Socket::operator=(ref);
  server_configs_ = ref.server_configs();
  transaction_ = ref.transaction();
  request_ = ref.request();
  response_ = ref.response();
  request_body_parsed_ = ref.request_body_parsed_;

  return *this;
}

void Client::ResetClientInfo() {
  transaction_ = Transaction();
  request_ = std::string();
  response_ = std::string();
  request_body_parsed_ = false;
}

ssize_t Client::ReceiveRequest() {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = recv(Socket::fd(), buffer, BUFFER_SIZE - 1, 0);
  if (bytes_read == -1) return bytes_read;

  buffer[BUFFER_SIZE - 1] = 0;
  request_ += buffer;

  return bytes_read;
}

int Client::ParseRequestHeader() {
  if (request_body_parsed_) return transaction_.status_code();
  request_body_parsed_ = true;

  int http_status = transaction_.ParseRequestHeader(request_);
  const std::string& host = transaction_.headers_in().host;
  const Server::Configuration& sc = this->conf_by_host(host);

  transaction_.uri_instance().ReconstructTargetUri(host);
  transaction_.GetConfiguration(sc);

  return http_status;
}

int Client::ParseRequestBody() {
  return transaction_.ParseRequestBody(
      request_body().c_str(), transaction_.headers_in().content_length_n);
}

void Client::CreateResponseMessage() {
  response_ = transaction_.CreateResponseMessage();
}

void Client::CreateResponseMessageByCgi() {
  std::string cgi_res;

  if (this->ReadCgi(cgi_res) == -1) {
    transaction_.set_status_code(HTTP_BAD_GATEWAY);
  } else {
    transaction_.cgi_instance().set_response(cgi_res);
  }

  response_ = transaction_.CreateResponseMessage();
}

int Client::SendResponseMessage() {
  return send(Socket::fd(), response_.c_str(), response_.length(), 0);
}

int Client::ReadCgi(std::string& cgi_res) {
  const Cgi& cgi = transaction_.cgi();

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  while ((bytes_read = read(cgi.cgi2server_fd()[0], buffer, BUFFER_SIZE - 1)) >
         0) {
    buffer[bytes_read] = 0;
    cgi_res += buffer;
  }
  close(cgi.cgi2server_fd()[0]);
  return bytes_read;
}

bool Client::IsReceiveRequestHeaderComplete() {
  if (request_.find(REQUEST_HEADER_END) == std::string::npos) {
    return false;
  }

  return true;
}

bool Client::IsReceiveRequestBodyComplete() {
  const HeadersIn& headers_in = transaction_.headers_in();
  const Transaction::Configuration& config = transaction_.config();
  const ssize_t& content_length = headers_in.content_length_n;
  const std::string& transfer_encoding = headers_in.transfer_encoding;
  const std::string& request_body = this->request_body();
  const size_t& client_max_body_size =
      static_cast<size_t>(config.client_max_body_size());

  if (!this->IsRequestBodyAllowedMethod()) {
    return true;
  }

  if (content_length >= 0) {
    if (request_body.length() > client_max_body_size) {
      transaction_.set_status_code(HTTP_REQUEST_ENTITY_TOO_LARGE);
    } else if ((ssize_t)request_body.length() < content_length) {
      return false;
    }
  } else if (transfer_encoding == "chunked") {
    if (request_body.find("0" CRLF) == std::string::npos) {
      return false;
    }
  } else {
    if (request_body.length() > 0) {
      transaction_.set_status_code(HTTP_LENGTH_REQUIRED);
    }
  }

  return true;
}

bool Client::IsRequestBodyAllowedMethod() {
  const std::string& method = transaction_.method();
  if (method == "GET" || method == "DELETE") {
    return false;
  }

  return true;
}

const Server::Configuration& Client::conf_by_host(const std::string& host) {
  for (Configuration::const_iterator it = server_configs_.begin();
       it != server_configs_.end(); it++) {
    if (it->server_names().find(host) != it->server_names().end()) {
      return *it;
    }
  }

  return server_configs_[0];
}

const std::string Client::request_body() {
  size_t offset = request_.find(REQUEST_HEADER_END);
  if (offset == std::string::npos) {
    return "";
  }

  return request_.substr(offset + std::strlen(REQUEST_HEADER_END));
}

const Server::Configurations& Client::server_configs() const {
  return server_configs_;
}
const std::string& Client::request() const { return request_; }
const std::string& Client::response() const { return response_; }
const Transaction& Client::transaction() const { return transaction_; }
Transaction& Client::transaction() { return transaction_; }

void Client::set_request(const std::string& request) { request_ = request; }
void Client::set_response(const std::string& response) { response_ = response; }
