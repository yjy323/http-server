#ifndef CONFIGURATION_PARSER_HPP
#define CONFIGURATION_PARSER_HPP

#include <set>
#include <string>

#include "configuration.hpp"
#include "core.hpp"

class ConfigurationParser {
 public:
  virtual ~ConfigurationParser();

  static int Parse(const std::string& contents, Configuration& configuration);

 private:
  static const std::string COMMENT_TOKEN;
  static const std::string END_TOKEN;
  static const std::string OPEN_BLOCK_TOKEN;
  static const std::string CLOSE_BLOCK_TOKEN;
  static const std::set<std::string> SINGLE_TOKEN;

  typedef std::string Token;
  typedef std::vector<Token> Tokens;

  ConfigurationParser();
  ConfigurationParser(const ConfigurationParser& ref);

  ConfigurationParser& operator=(const ConfigurationParser& ref);

  static int EraseComment(std::string& contents);
  static int Tokenize(const std::string& str, Tokens& tokens);
  static int Parse(const Tokens& tokens, Configuration& configuration);

  static int ParseServer(const Tokens& tokens,
                         ServerConfiguration& ServerConfiguration);
  static int ParseLocation(
      const Tokens& tokens, const ServerConfiguration& serverConfiguration,
      ServerConfiguration::LocationConfiguration& locationConfiguration);

  static int ParseServerLine(const Token& directive, const Tokens& valueTokens,
                             int& port, std::set<std::string>& server_names,
                             std::string& error_page, int& client_max_body_size,
                             std::string& root, bool& auto_index,
                             std::string& index);
  static int ParseLocationLine(const Token& directive,
                               const Tokens& valueTokens,
                               std::string& error_page,
                               int& client_max_body_size, std::string& root,
                               bool& auto_index, std::string& index,
                               std::set<std::string>& allowed_method,
                               std::string& return_uri,
                               std::string& upload_store);

  static int ParsePort(int& port, const Tokens& valueTokens);
  static int ParseServer_names(std::set<std::string>& server_names,
                               const Tokens& valueTokens);
  static int ParseError_page(std::string& error_page,
                             const Tokens& valueTokens);
  static int ParseClient_max_body_size(int& client_max_body_size,
                                       const Tokens& valueTokens);
  static int ParseRoot(std::string& root, const Tokens& valueTokens);
  static int ParseAuto_index(bool& auto_index, const Tokens& valueTokens);
  static int ParseIndex(std::string& index, const Tokens& valueTokens);
  static int ParseAllowed_method(std::set<std::string>& allowed_method,
                                 const Tokens& valueTokens);
  static int ParseReturn_uri(std::string& return_uri,
                             const Tokens& valueTokens);
  static int ParseUpload_store(std::string& upload_store,
                               const Tokens& valueTokens);

  static int ValidConfiguration(const Configuration& conf);
  static int ValidServerUnique(const Configuration& conf);

  static std::set<std::string> getDefaultServerName();
  static std::set<std::string> getDefaultAllowedMethod();

  static bool IsPort(const std::string& port);
  static bool IsAutoIndex(const std::string& auto_index);
};

#endif
