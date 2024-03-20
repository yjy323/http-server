#include "Configuration.hpp"

/* ServerConfiguration */

/** ServerConfiguration::constructor **/

ServerConfiguration::ServerConfiguration() {
  host = "127.0.0.1";
  port = 0;
  (void)server_names;
  (void)location;
  (void)error_page;
  client_max_body_size = "1M";
  (void)root;
  auto_index = false;
  (void)index_if_dir;
  index = "index.html";
}

ServerConfiguration::ServerConfiguration(
    const std::string& host, const int& port,
    const std::set<const std::string>& server_names,
    const std::map<const std::string, const LocationConfiguration>& location,
    const std::map<const int, const std::string>& error_page,
    const std::string& client_max_body_size, const std::string& root,
    const bool& auto_index, const std::string& index,
    const std::string& index_if_dir)
    : host(host),
      port(port),
      server_names(server_names),
      location(location),
      error_page(error_page),
      client_max_body_size(client_max_body_size),
      root(root),
      auto_index(auto_index),
      index(index),
      index_if_dir(index_if_dir) {}

ServerConfiguration::ServerConfiguration(const ServerConfiguration& ref) {
  *this = ref;
}

/** ServerConfiguration::destructor **/
ServerConfiguration::~ServerConfiguration() {}

/** ServerConfiguration::overator overriding **/

ServerConfiguration& ServerConfiguration::operator=(
    const ServerConfiguration& ref) {
  if (this == &ref) return *this;

  this->host = ref.getHost();
  this->port = ref.getPort();
  for (std::set<const std::string>::const_iterator it =
           ref.getServer_names().begin();
       it != ref.getServer_names().end(); it++) {
    this->server_names.insert(*it);
  }
  for (std::map<const std::string, const LocationConfiguration>::const_iterator
           it = ref.getLocation().begin();
       it != ref.getLocation().end(); it++) {
    this->location.insert(std::make_pair(it->first, it->second));
  }
  for (std::map<const int, const std::string>::const_iterator it =
           ref.getError_page().begin();
       it != ref.getError_page().end(); it++) {
    this->error_page.insert(std::make_pair(it->first, it->second));
  }
  this->client_max_body_size = ref.getClient_max_body_size();
  this->root = ref.getRoot();
  this->auto_index = ref.getAuto_index();
  this->index = ref.getIndex();
  this->index_if_dir = ref.getIndex_if_dir();

  return *this;
}

/** ServerConfiguration::getter **/

const std::string& ServerConfiguration::getHost() const { return this->host; }

const int& ServerConfiguration::getPort() const { return this->port; }

const std::set<const std::string>& ServerConfiguration::getServer_names()
    const {
  return this->server_names;
}

const std::map<const std::string,
               const ServerConfiguration::LocationConfiguration>&
ServerConfiguration::getLocation() const {
  return this->location;
}

const std::map<const int, const std::string>&
ServerConfiguration::getError_page() const {
  return this->error_page;
}

const std::string& ServerConfiguration::getClient_max_body_size() const {
  return this->client_max_body_size;
}

const std::string& ServerConfiguration::getRoot() const { return this->root; }

const bool& ServerConfiguration::getAuto_index() const {
  return this->auto_index;
}

const std::string& ServerConfiguration::getIndex_if_dir() const {
  return this->index_if_dir;
}

const std::string& ServerConfiguration::getIndex() const { return this->index; }

/* ServerConfiguration::LocationConfiguration */

/** ServerConfiguration::LocationConfiguration::constructor **/

ServerConfiguration::LocationConfiguration::LocationConfiguration() {}

ServerConfiguration::LocationConfiguration::LocationConfiguration(
    const std::map<const int, const std::string>& error_page,
    const std::string& client_max_body_size, const std::string& root,
    const bool& auto_index, const std::string& index,
    const std::string& index_if_dir,
    const std::set<const std::string>& allowed_method,
    const std::string& return_uri, const std::string& upload_store)
    : error_page(error_page),
      client_max_body_size(client_max_body_size),
      root(root),
      auto_index(auto_index),
      index(index),
      index_if_dir(index_if_dir),
      allowed_method(allowed_method),
      return_uri(return_uri),
      upload_store(upload_store) {}

ServerConfiguration::LocationConfiguration::LocationConfiguration(
    const LocationConfiguration& ref) {
  *this = ref;
}

/** ServerConfiguration::LocationConfiguration::destructor **/
ServerConfiguration::LocationConfiguration::~LocationConfiguration() {
  (void)error_page;
  client_max_body_size = "1M";
  (void)root;
  auto_index = false;
  index = "index.html";
  (void)index_if_dir;
  (void)allowed_method;
  (void)return_uri;
  upload_store = "upload_store/";
}

/** ServerConfiguration::LocationConfiguration::overator overriding **/
ServerConfiguration::LocationConfiguration&
ServerConfiguration::LocationConfiguration::operator=(
    const LocationConfiguration& ref) {
  if (this == &ref) return *this;

  for (std::map<const int, const std::string>::const_iterator it =
           ref.getError_page().begin();
       it != ref.getError_page().end(); it++) {
    this->error_page.insert(std::make_pair(it->first, it->second));
  }
  this->client_max_body_size = ref.getClient_max_body_size();
  this->root = ref.getRoot();
  this->auto_index = ref.getAuto_index();
  this->index = ref.getIndex();
  this->index_if_dir = ref.getIndex_if_dir();
  for (std::set<const std::string>::const_iterator it =
           ref.getAllowed_method().begin();
       it != ref.getAllowed_method().end(); it++) {
    this->allowed_method.insert(*it);
  }
  this->return_uri = ref.getReturn_uri();
  this->upload_store = ref.getUpload_store();

  return *this;
}

/** ServerConfiguration::LocationConfiguration::getter **/

const std::map<const int, const std::string>&
ServerConfiguration::LocationConfiguration::getError_page() const {
  return this->error_page;
}

const std::string&
ServerConfiguration::LocationConfiguration::getClient_max_body_size() const {
  return this->client_max_body_size;
}

const std::string& ServerConfiguration::LocationConfiguration::getRoot() const {
  return this->root;
}

const bool& ServerConfiguration::LocationConfiguration::getAuto_index() const {
  return this->auto_index;
}

const std::string& ServerConfiguration::LocationConfiguration::getIndex()
    const {
  return this->index;
}

const std::string& ServerConfiguration::LocationConfiguration::getIndex_if_dir()
    const {
  return this->index_if_dir;
}

const std::set<const std::string>&
ServerConfiguration::LocationConfiguration::getAllowed_method() const {
  return this->allowed_method;
}

const std::string& ServerConfiguration::LocationConfiguration::getReturn_uri()
    const {
  return this->return_uri;
}

const std::string& ServerConfiguration::LocationConfiguration::getUpload_store()
    const {
  return this->upload_store;
}

/** ServerConfiguration::io overator overriding **/

std::ostream& operator<<(std::ostream& out,
                         const ServerConfiguration& serverConfiguration) {
  out << "[Server Configuration]" << std::endl;

  out << "	host: " << serverConfiguration.getHost() << std::endl;

  out << "	port: " << serverConfiguration.getPort() << std::endl;

  out << "	server_names: " << std::endl;
  const std::set<const std::string>& server_names =
      serverConfiguration.getServer_names();
  for (std::set<const std::string>::const_iterator it = server_names.begin();
       it != server_names.end(); it++) {
    out << "		" << *it << std::endl;
  }

  out << "	error_page: " << std::endl;
  const std::map<const int, const std::string> error_page =
      serverConfiguration.getError_page();
  for (std::map<const int, const std::string>::const_iterator it =
           error_page.begin();
       it != error_page.end(); it++) {
    out << "		" << it->first << " : " << it->second << std::endl;
  }

  out << "	client_max_body_size: "
      << serverConfiguration.getClient_max_body_size() << std::endl;

  out << "	root: " << serverConfiguration.getRoot() << std::endl;

  out << "	auto_index: " << serverConfiguration.getAuto_index()
      << std::endl;

  out << "	index: " << serverConfiguration.getIndex_if_dir() << std::endl;

  out << "	index_if_dir: " << serverConfiguration.getIndex() << std::endl;

  out << "	locations in Server: " << std::endl;
  const std::map<const std::string,
                 const ServerConfiguration::LocationConfiguration>
      location = serverConfiguration.getLocation();
  for (std::map<const std::string,
                const ServerConfiguration::LocationConfiguration>::
           const_iterator it = location.begin();
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
  out << "			error_page: " << std::endl;

  const std::map<const int, const std::string> error_page =
      locationConfiguration.getError_page();
  for (std::map<const int, const std::string>::const_iterator it =
           error_page.begin();
       it != error_page.end(); it++) {
    out << "				" << it->first << " : " << it->second
        << std::endl;
  }

  out << "			client_max_body_size: "
      << locationConfiguration.getClient_max_body_size() << std::endl;

  out << "			root: " << locationConfiguration.getRoot()
      << std::endl;

  out << "			auto_index: "
      << locationConfiguration.getAuto_index() << std::endl;

  out << "			index: " << locationConfiguration.getIndex()
      << std::endl;

  out << "			index_if_dir: "
      << locationConfiguration.getIndex_if_dir() << std::endl;

  out << "			allowed_method: " << std::endl;
  const std::set<const std::string> allowed_method =
      locationConfiguration.getAllowed_method();
  for (std::set<const std::string>::const_iterator it = allowed_method.begin();
       it != allowed_method.end(); it++) {
    out << "				" << *it << std::endl;
  }

  out << "			return_uri: "
      << locationConfiguration.getReturn_uri() << std::endl;

  out << "			upload_store: "
      << locationConfiguration.getUpload_store() << std::endl;

  return out;
}
