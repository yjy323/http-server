#ifndef CONFIGURATION_PARSER_HPP
#define CONFIGURATION_PARSER_HPP

#include <set>
#include <string>

#include "configuration.hpp"
#include "core.hpp"

class ConfigurationParser {
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

  static Return eraseComment(std::string& contents);
  static Return tokenize(const std::string& str, Tokens& tokens);
  static Return parse(const Tokens& tokens, Configuration& configuration);

  static Return parseServer(const Tokens& tokens,
                            ServerConfiguration& ServerConfiguration);
  static Return parseLocation(
      const Tokens& tokens, const ServerConfiguration& serverConfiguration,
      ServerConfiguration::LocationConfiguration& locationConfiguration);

  static Return parseServerLine(
      const Token& directive, const Tokens& valueTokens, std::string& host,
      int& port, std::set<const std::string>& server_names,
      std::map<const int, const std::string>& error_page,
      std::string& client_max_body_size, std::string& root, bool& auto_index,
      std::string& index, std::string& index_if_dir);
  static Return parseLocationLine(
      const Token& directive, const Tokens& valueTokens,
      std::map<const int, const std::string>& error_page,
      std::string& client_max_body_size, std::string& root, bool& auto_index,
      std::string& index, std::string& index_if_dir,
      std::set<const std::string>& allowed_method, std::string& return_uri,
      std::string& upload_store);

  static Return parseHost(std::string& host, const Tokens& valueTokens);
  static Return parsePort(int& port, const Tokens& valueTokens);
  static Return parseServer_names(std::set<const std::string>& server_names,
                                  const Tokens& valueTokens);
  static Return parseError_page(
      std::map<const int, const std::string>& error_page,
      const Tokens& valueTokens);
  static Return parseClient_max_body_size(std::string& client_max_body_size,
                                          const Tokens& valueTokens);
  static Return parseRoot(std::string& root, const Tokens& valueTokens);
  static Return parseAuto_index(bool& auto_index, const Tokens& valueTokens);
  static Return parseIndex(std::string& index, const Tokens& valueTokens);
  static Return parseIndex_if_dir(std::string& index_if_dir,
                                  const Tokens& valueTokens);
  static Return parseAllowed_method(std::set<const std::string>& allowed_method,
                                    const Tokens& valueTokens);
  static Return parseReturn_uri(std::string& return_uri,
                                const Tokens& valueTokens);
  static Return parseUpload_store(std::string& upload_store,
                                  const Tokens& valueTokens);

  static bool isHost(const std::string& port);
  static bool isPort(const std::string& port);
  static bool isErrorCode(const std::string& error_code);
  static bool isAutoIndex(const std::string& auto_index);

 public:
  virtual ~ConfigurationParser();

  static Return parse(const std::string& contents,
                      Configuration& configuration);
};

#endif
