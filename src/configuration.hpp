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
  int port_;
  std::set<std::string> server_names_;
  std::map<std::string, LocationConfiguration> location_;
  std::map<int, std::string> error_page_;
  std::string client_max_body_size_;
  std::string root_;
  bool auto_index_;
  std::string index_;

 public:
  /* constructor */
  ServerConfiguration();
  ServerConfiguration(
      const int& port, const std::set<std::string>& server_names,
      const std::map<std::string, LocationConfiguration>& location,
      const std::map<int, std::string>& error_page,
      const std::string& client_max_body_size, const std::string& root,
      const bool& auto_index, const std::string& index);
  ServerConfiguration(const ServerConfiguration& ref);

  /* destructor */
  virtual ~ServerConfiguration();

  /* operator overriding */
  ServerConfiguration& operator=(const ServerConfiguration& ref);

  /* getter */
  const int& port() const;
  const std::set<std::string>& server_names() const;
  const std::map<std::string, LocationConfiguration>& location() const;
  const std::map<int, std::string>& error_page() const;
  const std::string& client_max_body_size() const;
  const std::string& root() const;
  const bool& auto_index() const;
  const std::string& index() const;

 public:
  /* inner class */
  class LocationConfiguration {
   private:
    /* variable */
    std::map<int, std::string> error_page_;
    std::string client_max_body_size_;
    std::string root_;
    bool auto_index_;
    std::string index_;
    std::set<std::string> allowed_method_;
    std::string return_uri_;
    std::string upload_store_;

   public:
    /* contructor */
    LocationConfiguration();
    LocationConfiguration(const std::map<int, std::string>& error_page,
                          const std::string& client_max_body_size,
                          const std::string& root, const bool& auto_index,
                          const std::string& index,
                          const std::set<std::string>& allowed_method,
                          const std::string& return_uri,
                          const std::string& upload_store);
    LocationConfiguration(const LocationConfiguration& ref);

    /* destructor */
    virtual ~LocationConfiguration();

    /* operator overriding */
    LocationConfiguration& operator=(const LocationConfiguration& ref);

    /* getter */
    const std::map<int, std::string>& error_page() const;
    const std::string& client_max_body_size() const;
    const std::string& root() const;
    const bool& auto_index() const;
    const std::string& index() const;
    const std::set<std::string>& allowed_method() const;
    const std::string& return_uri() const;
    const std::string& upload_store() const;
  };
};

std::ostream& operator<<(std::ostream& out,
                         const ServerConfiguration& serverConfiguration);
std::ostream& operator<<(
    std::ostream& out,
    const ServerConfiguration::LocationConfiguration& locationConfiguration);

#endif
