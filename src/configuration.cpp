#include "configuration.hpp"

/* ServerConfiguration::LocationConfiguration */

/** ServerConfiguration::LocationConfiguration::constructor **/

ServerConfiguration::LocationConfiguration::LocationConfiguration()
    : error_page_(),
      client_max_body_size_(),
      root_(),
      auto_index_(),
      index_(),
      allowed_method_(),
      return_uri_(),
      upload_store_() {}

ServerConfiguration::LocationConfiguration::LocationConfiguration(
    const std::string& error_page, const int& client_max_body_size,
    const std::string& root, const bool& auto_index, const std::string& index,
    const std::set<std::string>& allowed_method, const std::string& return_uri,
    const std::string& upload_store)
    : error_page_(error_page),
      client_max_body_size_(client_max_body_size),
      root_(root),
      auto_index_(auto_index),
      index_(index),
      allowed_method_(allowed_method),
      return_uri_(return_uri),
      upload_store_(upload_store) {}

ServerConfiguration::LocationConfiguration::LocationConfiguration(
    const LocationConfiguration& ref) {
  *this = ref;
}

/** ServerConfiguration::LocationConfiguration::destructor **/
ServerConfiguration::LocationConfiguration::~LocationConfiguration() {}

/** ServerConfiguration::LocationConfiguration::operator overriding **/
ServerConfiguration::LocationConfiguration&
ServerConfiguration::LocationConfiguration::operator=(
    const LocationConfiguration& ref) {
  if (this == &ref) return *this;

  this->error_page_ = ref.error_page();
  this->client_max_body_size_ = ref.client_max_body_size();
  this->root_ = ref.root();
  this->auto_index_ = ref.auto_index();
  this->index_ = ref.index();
  for (std::set<std::string>::const_iterator it = ref.allowed_method().begin();
       it != ref.allowed_method().end(); it++) {
    this->allowed_method_.insert(*it);
  }
  this->return_uri_ = ref.return_uri();
  this->upload_store_ = ref.upload_store();

  return *this;
}

/** ServerConfiguration::LocationConfiguration::getter **/

const std::string& ServerConfiguration::LocationConfiguration::error_page()
    const {
  return this->error_page_;
}

const int& ServerConfiguration::LocationConfiguration::client_max_body_size()
    const {
  return this->client_max_body_size_;
}

const std::string& ServerConfiguration::LocationConfiguration::root() const {
  return this->root_;
}

const bool& ServerConfiguration::LocationConfiguration::auto_index() const {
  return this->auto_index_;
}

const std::string& ServerConfiguration::LocationConfiguration::index() const {
  return this->index_;
}

const std::set<std::string>&
ServerConfiguration::LocationConfiguration::allowed_method() const {
  return this->allowed_method_;
}

const std::string& ServerConfiguration::LocationConfiguration::return_uri()
    const {
  return this->return_uri_;
}

const std::string& ServerConfiguration::LocationConfiguration::upload_store()
    const {
  return this->upload_store_;
}

/* ServerConfiguration */

/** ServerConfiguration::constructor **/

ServerConfiguration::ServerConfiguration()
    : port_(),
      server_names_(),
      location_(),
      error_page_(),
      client_max_body_size_(),
      root_(),
      auto_index_(),
      index_() {}

ServerConfiguration::ServerConfiguration(
    const int& port, const std::set<std::string>& server_names,
    const std::map<std::string, LocationConfiguration>& location,
    const std::string& error_page, const int& client_max_body_size,
    const std::string& root, const bool& auto_index, const std::string& index)
    : port_(port),
      server_names_(server_names),
      location_(location),
      error_page_(error_page),
      client_max_body_size_(client_max_body_size),
      root_(root),
      auto_index_(auto_index),
      index_(index) {}

ServerConfiguration::ServerConfiguration(const ServerConfiguration& ref) {
  *this = ref;
}

/** ServerConfiguration::destructor **/
ServerConfiguration::~ServerConfiguration() {}

/** ServerConfiguration::operator overriding **/

ServerConfiguration& ServerConfiguration::operator=(
    const ServerConfiguration& ref) {
  if (this == &ref) return *this;

  this->port_ = ref.port();

  for (std::set<std::string>::const_iterator it = ref.server_names().begin();
       it != ref.server_names().end(); it++) {
    this->server_names_.insert(*it);
  }
  for (std::map<std::string, LocationConfiguration>::const_iterator it =
           ref.location().begin();
       it != ref.location().end(); it++) {
    this->location_.insert(std::make_pair(it->first, it->second));
  }
  this->error_page_ = ref.error_page();
  this->client_max_body_size_ = ref.client_max_body_size();
  this->root_ = ref.root();
  this->auto_index_ = ref.auto_index();
  this->index_ = ref.index();

  return *this;
}

/** ServerConfiguration::getter **/

const int& ServerConfiguration::port() const { return this->port_; }

const std::set<std::string>& ServerConfiguration::server_names() const {
  return this->server_names_;
}

const std::map<std::string, ServerConfiguration::LocationConfiguration>&
ServerConfiguration::location() const {
  return this->location_;
}

const std::string& ServerConfiguration::error_page() const {
  return this->error_page_;
}

const int& ServerConfiguration::client_max_body_size() const {
  return this->client_max_body_size_;
}

const std::string& ServerConfiguration::root() const { return this->root_; }

const bool& ServerConfiguration::auto_index() const {
  return this->auto_index_;
}

const std::string& ServerConfiguration::index() const { return this->index_; }

/** ServerConfiguration::io overator overriding **/

std::ostream& operator<<(std::ostream& out,
                         const ServerConfiguration& serverConfiguration) {
  out << "[Server Configuration]" << std::endl;

  out << "	port: " << serverConfiguration.port() << std::endl;

  out << "	server_names: " << std::endl;
  const std::set<std::string>& server_names =
      serverConfiguration.server_names();
  for (std::set<std::string>::const_iterator it = server_names.begin();
       it != server_names.end(); it++) {
    out << "		" << *it << std::endl;
  }

  out << "	error_page: " << serverConfiguration.error_page() << std::endl;

  out << "	client_max_body_size: "
      << serverConfiguration.client_max_body_size() << std::endl;

  out << "	root: " << serverConfiguration.root() << std::endl;

  out << "	auto_index: " << serverConfiguration.auto_index() << std::endl;

  out << "	index_if_dir: " << serverConfiguration.index() << std::endl;

  out << "	locations in Server: " << std::endl;
  const std::map<std::string, ServerConfiguration::LocationConfiguration>
      location = serverConfiguration.location();
  for (std::map<std::string,
                ServerConfiguration::LocationConfiguration>::const_iterator it =
           location.begin();
       it != location.end(); it++) {
    out << "		[" << it->first << " Location Configuration]"
        << std::endl
        << it->second << std::endl;
  }

  return out;
}

/** ServerConfiguration::LocationConfiguration::io overator overriding **/

std::ostream& operator<<(
    std::ostream& out,
    const ServerConfiguration::LocationConfiguration& locationConfiguration) {
  out << "			error_page: "
      << locationConfiguration.error_page() << std::endl;

  out << "			client_max_body_size: "
      << locationConfiguration.client_max_body_size() << std::endl;

  out << "			root: " << locationConfiguration.root()
      << std::endl;

  out << "			auto_index: "
      << locationConfiguration.auto_index() << std::endl;

  out << "			index: " << locationConfiguration.index()
      << std::endl;

  out << "			allowed_method: " << std::endl;
  const std::set<std::string> allowed_method =
      locationConfiguration.allowed_method();
  for (std::set<std::string>::const_iterator it = allowed_method.begin();
       it != allowed_method.end(); it++) {
    out << "				" << *it << std::endl;
  }

  out << "			return_uri: "
      << locationConfiguration.return_uri() << std::endl;

  out << "			upload_store: "
      << locationConfiguration.upload_store() << std::endl;

  return out;
}
