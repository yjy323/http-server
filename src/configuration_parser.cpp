#include "configuration_parser.hpp"

#include <cstdlib>
#include <sstream>

#include "file_reader.hpp"
#include "utils.hpp"

#define DEFAULT_PORT 8080
#define DEFAULT_SERVER_NAMES ConfigurationParser::getDefaultServerName()
#define DEFAULT_LOCATION \
  std::map<std::string, ServerConfiguration::LocationConfiguration>()
#define DEFAULT_ERROR_PAGE ""
#define DEFAULT_CLIENT_MAX_BODY_SIZE 1000000
#define DEFAULT_SERVER_ROOT "/www"
#define EFAULT_LOCATION_ROOT ""
#define DEFAULT_AUTO_INDEX false
#define DEFAULT_INDEX ""
#define DEFAULT_ALLOWED_METHOD ConfigurationParser::getDefaultAllowedMethod()
#define DEFAULT_RETURN_URI ""
#define DEFAULT_UPLOAD_STORE "/upload"

#define PARSE_ERROR_MESSAGE "an not parse the configuration file"
#define PORT_HOST_VALID_ERROR_MESSAGE \
  "The combination of the server's port and server_name must be unique."

const std::string ConfigurationParser::COMMENT_TOKEN = "#";
const std::string ConfigurationParser::END_TOKEN = ";";
const std::string ConfigurationParser::OPEN_BLOCK_TOKEN = "{";
const std::string ConfigurationParser::CLOSE_BLOCK_TOKEN = "}";

ConfigurationParser::~ConfigurationParser() {}

int ConfigurationParser::Parse(const std::string& contents,
                               Configuration& configuration) {
  std::string contents_copy(contents);
  Tokens tokens;

  if (EraseComment(contents_copy) == ERROR ||
      Tokenize(contents_copy, tokens) == ERROR ||
      Parse(tokens, configuration) == ERROR) {
    return ERROR;
  }

  if (ValidConfiguration(configuration) == ERROR) {
    return ERROR;
  }

  return OK;
}

int ConfigurationParser::EraseComment(std::string& contents) {
  size_t pos = 0;

  while ((pos = contents.find(COMMENT_TOKEN, pos)) != std::string::npos) {
    size_t endLine = contents.find('\n', pos);

    if (endLine == std::string::npos) {
      contents.erase(pos);
      break;
    } else {
      contents.erase(pos, endLine - pos);
    }
  }

  return OK;
}

int ConfigurationParser::Tokenize(const std::string& str,
                                  ConfigurationParser::Tokens& tokens) {
  Token token;

  for (size_t idx = 0; idx < str.length(); idx++) {
    if (std::isspace(str[idx])) {
      if (token.length() != 0) {
        tokens.push_back(token);
        token = "";
      }

      continue;
    }

    if (str[idx] == END_TOKEN[0] || str[idx] == CLOSE_BLOCK_TOKEN[0] ||
        OPEN_BLOCK_TOKEN[0] == '}') {
      if (token.length() != 0) {
        tokens.push_back(token);
        token = "";
      }

      tokens.push_back(str.substr(idx, 1));
      continue;
    }

    token.append(1, str[idx]);
  }

  if (token.length() != 0) {
    tokens.push_back(token);
  }

  return OK;
}

int ConfigurationParser::Parse(const Tokens& tokens,
                               Configuration& configuration) {
  Tokens serverTokens;
  unsigned int contextDepth = 0;
  bool blockOpenFlag = false;
  for (size_t idx = 0; idx < tokens.size(); idx++) {
    const Token token = tokens[idx];

    if (blockOpenFlag && token != OPEN_BLOCK_TOKEN) return ERROR;

    if (token == "server") {
      blockOpenFlag = true;
      continue;
    }

    if (token == OPEN_BLOCK_TOKEN) {
      contextDepth++;

      if (contextDepth == 1) {
        blockOpenFlag = false;
        continue;
      }
    }

    if (token == CLOSE_BLOCK_TOKEN) {
      if (contextDepth == 0) return ERROR;
      contextDepth--;

      if (contextDepth == 0) {
        ServerConfiguration serverConfiguration;

        if (ParseServer(serverTokens, serverConfiguration) == ERROR)
          return ERROR;

        configuration.push_back(serverConfiguration);
        serverTokens.clear();
        continue;
      }
    }

    if (contextDepth == 0) return ERROR;

    serverTokens.push_back(token);
  }

  if (contextDepth != 0 || blockOpenFlag) return ERROR;

  return OK;
}

int ConfigurationParser::ParseServer(const Tokens& tokens,
                                     ServerConfiguration& serverConfiguration) {
  int port = DEFAULT_PORT;
  std::set<std::string> server_names;
  std::map<std::string, ServerConfiguration::LocationConfiguration> location =
      DEFAULT_LOCATION;
  std::string error_page = DEFAULT_ERROR_PAGE;
  int client_max_body_size = DEFAULT_CLIENT_MAX_BODY_SIZE;
  std::string root = DEFAULT_SERVER_ROOT;
  bool auto_index = DEFAULT_AUTO_INDEX;
  std::string index = DEFAULT_INDEX;

  std::set<std::string> parsed_directive;
  std::map<std::string, Tokens> locationTokenByPath;
  std::string locationPath;
  Tokens locationTokens;
  std::string directive;
  Tokens valueTokens;
  unsigned int contextDepth = 0;
  bool blockOpenFlag = false;
  bool locationPathFlag = false;
  bool locationBlockFlag = false;

  for (size_t idx = 0; idx < tokens.size(); idx++) {
    const Token token = tokens[idx];

    if (token == END_TOKEN) {
      if (locationPathFlag || blockOpenFlag) return ERROR;
      if (locationBlockFlag) {
        locationTokens.push_back(token);
        continue;
      }
      if (directive == "") continue;
      if (ParseServerLine(directive, valueTokens, port, server_names,
                          error_page, client_max_body_size, root, auto_index,
                          index) == ERROR)
        return ERROR;
      if (parsed_directive.find(directive) != parsed_directive.end()) {
        return ERROR;
      }
      parsed_directive.insert(directive);
      valueTokens.clear();
      directive = "";
      continue;
    }

    if (token == CLOSE_BLOCK_TOKEN) {
      if (locationPathFlag || blockOpenFlag || contextDepth == 0) return ERROR;
      contextDepth--;

      if (contextDepth == 0) {
        locationTokenByPath[locationPath] = locationTokens;

        locationTokens.clear();
        locationBlockFlag = false;
        locationPath = "";
        continue;
      }
    }
    if (token == OPEN_BLOCK_TOKEN) {
      contextDepth++;

      if (blockOpenFlag) {
        blockOpenFlag = false;
        locationBlockFlag = true;
        continue;
      }
    }
    if (token == "location") {
      if (locationPathFlag || blockOpenFlag) return ERROR;
      locationPathFlag = true;
      continue;
    }
    if (locationPathFlag) {
      locationPath = token;
      locationPathFlag = false;
      blockOpenFlag = true;
      continue;
    }
    if (locationBlockFlag) {
      locationTokens.push_back(token);
    } else if (directive == "") {
      directive = token;
    } else {
      valueTokens.push_back(token);
    }
  }

  if (directive != "" || contextDepth != 0 || blockOpenFlag ||
      locationPathFlag || locationBlockFlag)
    return ERROR;

  ServerConfiguration serverConfigurationForLocation(
      port, server_names, location, error_page, client_max_body_size, root,
      auto_index, index);

  for (std::map<std::string, Tokens>::const_iterator iter =
           locationTokenByPath.begin();
       iter != locationTokenByPath.end(); iter++) {
    ServerConfiguration::LocationConfiguration locationConfiguration;
    if (ParseLocation(iter->second, serverConfigurationForLocation,
                      locationConfiguration) == ERROR)
      return ERROR;

    location.insert(std::make_pair(iter->first, locationConfiguration));
  }

  if (server_names.size() == 0) server_names = DEFAULT_SERVER_NAMES;

  serverConfiguration =
      ServerConfiguration(port, server_names, location, error_page,
                          client_max_body_size, root, auto_index, index);

  return OK;
}

int ConfigurationParser::ParseLocation(
    const Tokens& tokens, const ServerConfiguration& serverConfiguration,
    ServerConfiguration::LocationConfiguration& locationConfiguartion) {
  std::string error_page = serverConfiguration.error_page();
  int client_max_body_size = serverConfiguration.client_max_body_size();
  std::string root = EFAULT_LOCATION_ROOT;
  bool auto_index = serverConfiguration.auto_index();
  std::string index = serverConfiguration.index();
  std::set<std::string> allowed_method;
  std::string return_uri = DEFAULT_RETURN_URI;
  std::string upload_store = DEFAULT_UPLOAD_STORE;

  std::set<std::string> parsed_directive;
  Token directive;
  Tokens valueTokens;

  for (size_t idx = 0; idx < tokens.size(); idx++) {
    const Token token = tokens[idx];

    if (token == END_TOKEN) {
      if (ParseLocationLine(directive, valueTokens, error_page,
                            client_max_body_size, root, auto_index, index,
                            allowed_method, return_uri, upload_store) == ERROR)
        return ERROR;
      if (parsed_directive.find(directive) != parsed_directive.end()) {
        return ERROR;
      }
      parsed_directive.insert(directive);
      directive = "";
      valueTokens.clear();

      continue;
    }

    if (token == CLOSE_BLOCK_TOKEN || token == OPEN_BLOCK_TOKEN) return ERROR;

    if (directive == "") {
      directive = token;
    } else {
      valueTokens.push_back(token);
    }
  }

  if (directive != "") return ERROR;

  if (allowed_method.size() == 0) allowed_method = DEFAULT_ALLOWED_METHOD;

  locationConfiguartion = ServerConfiguration::LocationConfiguration(
      error_page, client_max_body_size, root, auto_index, index, allowed_method,
      return_uri, upload_store);

  return OK;
}

int ConfigurationParser::ParseServerLine(const Token& directive,
                                         const Tokens& valueTokens, int& port,
                                         std::set<std::string>& server_names,
                                         std::string& error_page,
                                         int& client_max_body_size,
                                         std::string& root, bool& auto_index,
                                         std::string& index) {
  int rtn_parse = OK;

  if (directive == "listen") {
    rtn_parse = ParsePort(port, valueTokens);
  } else if (directive == "server_name") {
    rtn_parse = ParseServer_names(server_names, valueTokens);
  } else if (directive == "error_page") {
    rtn_parse = ParseError_page(error_page, valueTokens);
  } else if (directive == "client_max_body_size") {
    rtn_parse = ParseClient_max_body_size(client_max_body_size, valueTokens);
  } else if (directive == "root") {
    rtn_parse = ParseRoot(root, valueTokens);
  } else if (directive == "auto_index") {
    rtn_parse = ParseAuto_index(auto_index, valueTokens);
  } else if (directive == "index") {
    rtn_parse = ParseIndex(index, valueTokens);
  } else {
    return ERROR;
  }

  return rtn_parse;
}

int ConfigurationParser::ParseLocationLine(
    const Token& directive, const Tokens& valueTokens, std::string& error_page,
    int& client_max_body_size, std::string& root, bool& auto_index,
    std::string& index, std::set<std::string>& allowed_method,
    std::string& return_uri, std::string& upload_store) {
  int rtn_parse = OK;

  if (directive == "error_page") {
    rtn_parse = ParseError_page(error_page, valueTokens);
  } else if (directive == "client_max_body_size") {
    rtn_parse = ParseClient_max_body_size(client_max_body_size, valueTokens);
  } else if (directive == "root") {
    rtn_parse = ParseRoot(root, valueTokens);
  } else if (directive == "auto_index") {
    rtn_parse = ParseAuto_index(auto_index, valueTokens);
  } else if (directive == "index") {
    rtn_parse = ParseIndex(index, valueTokens);
  } else if (directive == "allowed_method") {
    rtn_parse = ParseAllowed_method(allowed_method, valueTokens);
  } else if (directive == "return") {
    rtn_parse = ParseReturn_uri(return_uri, valueTokens);
  } else if (directive == "upload_store") {
    rtn_parse = ParseUpload_store(upload_store, valueTokens);
  } else {
    return ERROR;
  }

  return rtn_parse;
}

int ConfigurationParser::ParsePort(int& port, const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;
  if (!IsPort(valueTokens[0])) return ERROR;

  std::stringstream(valueTokens[0]) >> port;

  return OK;
}

int ConfigurationParser::ParseServer_names(std::set<std::string>& server_names,
                                           const Tokens& valueTokens) {
  if (valueTokens.size() < 1) return ERROR;

  for (size_t idx = 0; idx < valueTokens.size(); idx++) {
    server_names.insert(valueTokens[idx]);
  }

  return OK;
}

int ConfigurationParser::ParseError_page(std::string& error_page,
                                         const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  error_page = valueTokens[0];

  return OK;
}

int ConfigurationParser::ParseClient_max_body_size(int& client_max_body_size,
                                                   const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  if (!isPositiveInteger(valueTokens[0])) return ERROR;

  client_max_body_size = std::atoi(valueTokens[0].c_str());

  return OK;
}

int ConfigurationParser::ParseRoot(std::string& root,
                                   const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  root = valueTokens[0];

  return OK;
}

int ConfigurationParser::ParseAuto_index(bool& auto_index,
                                         const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;
  if (!IsAutoIndex(valueTokens[0])) return ERROR;

  auto_index = (valueTokens[0] == "on" ? true : false);

  return OK;
}

int ConfigurationParser::ParseIndex(std::string& index,
                                    const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  index = valueTokens[0];

  return OK;
}

int ConfigurationParser::ParseAllowed_method(
    std::set<std::string>& allowed_method, const Tokens& valueTokens) {
  allowed_method.clear();

  if (valueTokens.size() < 1) return ERROR;

  for (size_t idx_value = 0; idx_value < valueTokens.size(); idx_value++) {
    allowed_method.insert(valueTokens[idx_value]);
  }

  return OK;
}

int ConfigurationParser::ParseReturn_uri(std::string& return_uri,
                                         const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  return_uri = valueTokens[0];

  return OK;
}

int ConfigurationParser::ParseUpload_store(std::string& upload_store,
                                           const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  upload_store = valueTokens[0];

  return OK;
}

int ConfigurationParser::ValidConfiguration(const Configuration& conf) {
  if (ValidServerUnique(conf) == ERROR) return ERROR;

  return OK;
}

int ConfigurationParser::ValidServerUnique(const Configuration& conf) {
  std::map<int, std::set<std::string> > server_names;
  std::vector<std::pair<int, std::string> > v_port_server_name;

  for (Configuration::const_iterator it_conf = conf.begin();
       it_conf != conf.end(); it_conf++) {
    for (std::set<std::string>::const_iterator it_server_names =
             it_conf->server_names().begin();
         it_server_names != it_conf->server_names().end(); it_server_names++) {
      if (server_names.find(it_conf->port()) == server_names.end()) {
        server_names[it_conf->port()] = std::set<std::string>();
        server_names[it_conf->port()].insert(*it_server_names);
      } else {
        if (server_names[it_conf->port()].find(*it_server_names) !=
            server_names[it_conf->port()].end()) {
          return ERROR;
        } else {
          server_names[it_conf->port()].insert(*it_server_names);
        }
      }
    }
  }

  return OK;
}

std::set<std::string> ConfigurationParser::getDefaultServerName() {
  std::set<std::string> server_names;

  server_names.insert("localhost");

  return server_names;
}

std::set<std::string> ConfigurationParser::getDefaultAllowedMethod() {
  std::set<std::string> allowed_method;

  allowed_method.insert("GET");
  allowed_method.insert("POST");

  return allowed_method;
}

bool ConfigurationParser::IsPort(const std::string& port) {
  int num = 0;

  for (size_t idx = 0; idx < port.size(); idx++) {
    if (!std::isdigit(port[idx])) return false;

    num = num * 10 + (port[idx] - '0');
  }

  if (port.length() > 5 || num > 65535) return false;

  return true;
}

bool ConfigurationParser::IsAutoIndex(const std::string& auto_index) {
  if (auto_index == "on" || auto_index == "off") return true;

  return false;
}
