#ifndef URI_HPP
#define URI_HPP

#include <string>

#include "http.hpp"

class Uri {
 public:
  enum RequestTargetFrom {
    kOriginForm,
    kAbsoluteForm,
    kAuthorityForm,
    kAsteriskForm
  };

  Uri();
  Uri(const Uri& obj);
  ~Uri();
  Uri& operator=(const Uri& obj);

  int ParseUriComponent(std::string request_uri);
  int ReconstructTargetUri(std::string request_host);

  RequestTargetFrom request_target_form() const;
  const std::string& request_target() const;
  const std::string& decoded_request_target() const;
  const std::string& scheme() const;
  const std::string& user() const;
  const std::string& password() const;
  const std::string& host() const;
  const std::string& path() const;
  const std::string& query_string() const;

 private:
  int ParseScheme(std::string& scheme);
  int ParseAuthority(std::string& authority);
  int ParsePathSegment(std::string& path_component);
  int ParseQuery(std::string& query);
  std::string DecodeRequestTarget();

  RequestTargetFrom request_target_form_;
  std::string request_target_;
  std::string decoded_request_target_;

  std::string scheme_;
  std::string user_;
  std::string password_;
  std::string host_;
  std::string path_;
  std::string query_string_;
};

#endif
