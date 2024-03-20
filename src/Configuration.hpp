#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#define DEFAULT_PORT 80
#define DEFAULT_AUTO_INDEX false

class ServerConfiguration;
typedef std::vector<ServerConfiguration> Configuration;

class ServerConfiguration {
 public:
  /* Declaration */
  class LocationConfiguration;

 private:
  /* variable */
  std::string host;
  int port;
  std::set<const std::string> server_names;
  std::map<const std::string, const LocationConfiguration> location;
  std::map<const int, const std::string> error_page;
  std::string client_max_body_size;
  std::string root;
  bool auto_index;
  std::string index;
  std::string index_if_dir;

 public:
  /* constructor */
  ServerConfiguration();
  ServerConfiguration(
      const std::string& host, const int& port,
      const std::set<const std::string>& server_names,
      const std::map<const std::string, const LocationConfiguration>& location,
      const std::map<const int, const std::string>& error_page,
      const std::string& client_max_body_size, const std::string& root,
      const bool& auto_index, const std::string& index,
      const std::string& index_if_dir);
  ServerConfiguration(const ServerConfiguration& ref);

  /* destructor */
  virtual ~ServerConfiguration();

  /* overator overriding */
  ServerConfiguration& operator=(const ServerConfiguration& ref);

  /* getter */
  const std::string& getHost() const;
  const int& getPort() const;
  const std::set<const std::string>& getServer_names() const;
  const std::map<const std::string, const LocationConfiguration>& getLocation()
      const;
  const std::map<const int, const std::string>& getError_page() const;
  const std::string& getClient_max_body_size() const;
  const std::string& getRoot() const;
  const bool& getAuto_index() const;
  const std::string& getIndex_if_dir() const;
  const std::string& getIndex() const;

 public:
  /* inner class */
  class LocationConfiguration {
   private:
    /* variable */
    std::map<const int, const std::string> error_page;
    std::string client_max_body_size;
    std::string root;
    bool auto_index;
    std::string index;
    std::string index_if_dir;
    std::set<const std::string> allowed_method;
    std::string return_uri;
    std::string upload_store;

   public:
    /* contructor */
    LocationConfiguration();
    LocationConfiguration(
        const std::map<const int, const std::string>& error_page,
        const std::string& client_max_body_size, const std::string& root,
        const bool& auto_index, const std::string& index,
        const std::string& index_if_dir,
        const std::set<const std::string>& allowed_method,
        const std::string& return_uri, const std::string& upload_store);
    LocationConfiguration(const LocationConfiguration& ref);

    /* destructor */
    virtual ~LocationConfiguration();

    /* overator overriding */
    LocationConfiguration& operator=(const LocationConfiguration& ref);

    /* getter */
    const std::map<const int, const std::string>& getError_page() const;
    const std::string& getClient_max_body_size() const;
    const std::string& getRoot() const;
    const bool& getAuto_index() const;
    const std::string& getIndex() const;
    const std::string& getIndex_if_dir() const;
    const std::set<const std::string>& getAllowed_method() const;
    const std::string& getReturn_uri() const;
    const std::string& getUpload_store() const;
  };
};

std::ostream& operator<<(std::ostream& out,
                         const ServerConfiguration& serverConfiguration);
std::ostream& operator<<(
    std::ostream& out,
    const ServerConfiguration::LocationConfiguration& locationConfiguration);

#endif
