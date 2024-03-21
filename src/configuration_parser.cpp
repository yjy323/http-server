#include "configuration_parser.hpp"

#include <fcntl.h>

#include <fstream>
#include <sstream>

#include "file_reader.hpp"
#include "utils.hpp"

const std::string ConfigurationParser::COMMENT_TOKEN = "#";
const std::string ConfigurationParser::END_TOKEN = ";";
const std::string ConfigurationParser::OPEN_BLOCK_TOKEN = "{";
const std::string ConfigurationParser::CLOSE_BLOCK_TOKEN = "}";

ConfigurationParser::~ConfigurationParser() {}

int ConfigurationParser::parse(const std::string& contents,
                               Configuration& configuration) {
  std::string contents_copy(contents);
  Tokens tokens;

  if (eraseComment(contents_copy) == ERROR ||
      tokenize(contents_copy, tokens) == ERROR ||
      parse(tokens, configuration) == ERROR) {
    return ERROR;
  }

  return OK;
}

int ConfigurationParser::eraseComment(std::string& contents) {
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

int ConfigurationParser::tokenize(const std::string& str,
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

int ConfigurationParser::parse(const Tokens& tokens,
                               Configuration& configuration) {
  Tokens serverTokens;
  uint64_t contextDepth = 0;
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

        if (parseServer(serverTokens, serverConfiguration) == ERROR)
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

int ConfigurationParser::parseServer(const Tokens& tokens,
                                     ServerConfiguration& serverConfiguration) {
  std::string host;
  int port;
  std::set<const std::string> server_names;
  std::map<const std::string, const ServerConfiguration::LocationConfiguration>
      location;
  std::map<const int, const std::string> error_page;
  std::string client_max_body_size;
  std::string root;
  bool auto_index;
  std::string index;
  std::string index_if_dir;

  std::map<std::string, Tokens> locationTokenByPath;
  std::string locationPath;
  Tokens locationTokens;
  std::string directive;
  Tokens valueTokens;
  uint64_t contextDepth = 0;
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
      if (parseServerLine(directive, valueTokens, host, port, server_names,
                          error_page, client_max_body_size, root, auto_index,
                          index, index_if_dir) == ERROR)
        return ERROR;
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
      host, port, server_names, location, error_page, client_max_body_size,
      root, auto_index, index, index_if_dir);

  for (std::map<std::string, Tokens>::const_iterator iter =
           locationTokenByPath.begin();
       iter != locationTokenByPath.end(); iter++) {
    ServerConfiguration::LocationConfiguration locationConfiguration;
    if (parseLocation(iter->second, serverConfigurationForLocation,
                      locationConfiguration) == ERROR)
      return ERROR;

    location.insert(std::make_pair(iter->first, locationConfiguration));
  }

  serverConfiguration = ServerConfiguration(
      host, port, server_names, location, error_page, client_max_body_size,
      root, auto_index, index, index_if_dir);

  return OK;
}

int ConfigurationParser::parseLocation(
    const Tokens& tokens, const ServerConfiguration& serverConfiguration,
    ServerConfiguration::LocationConfiguration& locationConfiguartion) {
  std::map<const int, const std::string> error_page =
      serverConfiguration.error_page();
  std::string client_max_body_size = serverConfiguration.client_max_body_size();
  std::string root = serverConfiguration.root();
  bool auto_index = serverConfiguration.auto_index();
  std::string index = serverConfiguration.index();
  std::string index_if_dir = serverConfiguration.index_if_dir();
  std::set<const std::string> allowed_method;
  std::string return_uri;
  std::string upload_store;

  Token directive;
  Tokens valueTokens;

  for (size_t idx = 0; idx < tokens.size(); idx++) {
    const Token token = tokens[idx];

    if (token == END_TOKEN) {
      if (parseLocationLine(directive, valueTokens, error_page,
                            client_max_body_size, root, auto_index, index,
                            index_if_dir, allowed_method, return_uri,
                            upload_store) == ERROR)
        return ERROR;
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

  locationConfiguartion = ServerConfiguration::LocationConfiguration(
      error_page, client_max_body_size, root, auto_index, index, index_if_dir,
      allowed_method, return_uri, upload_store);

  return OK;
}

int ConfigurationParser::parseServerLine(
    const Token& directive, const Tokens& valueTokens, std::string& host,
    int& port, std::set<const std::string>& server_names,
    std::map<const int, const std::string>& error_page,
    std::string& client_max_body_size, std::string& root, bool& auto_index,
    std::string& index, std::string& index_if_dir) {
  int rtn_parse = OK;

  if (directive == "host") {
    rtn_parse = parseHost(host, valueTokens);
  } else if (directive == "port") {
    rtn_parse = parsePort(port, valueTokens);
  } else if (directive == "server_name") {
    rtn_parse = parseServer_names(server_names, valueTokens);
  } else if (directive == "error_page") {
    rtn_parse = parseError_page(error_page, valueTokens);
  } else if (directive == "client_max_body_size") {
    rtn_parse = parseClient_max_body_size(client_max_body_size, valueTokens);
  } else if (directive == "root") {
    rtn_parse = parseRoot(root, valueTokens);
  } else if (directive == "auto_index") {
    rtn_parse = parseAuto_index(auto_index, valueTokens);
  } else if (directive == "index") {
    rtn_parse = parseIndex(index, valueTokens);
  } else if (directive == "index_if_dir") {
    rtn_parse = parseIndex_if_dir(index_if_dir, valueTokens);
  } else {
    return ERROR;
  }

  return rtn_parse;
}

int ConfigurationParser::parseLocationLine(
    const Token& directive, const Tokens& valueTokens,
    std::map<const int, const std::string>& error_page,
    std::string& client_max_body_size, std::string& root, bool& auto_index,
    std::string& index, std::string& index_if_dir,
    std::set<const std::string>& allowed_method, std::string& return_uri,
    std::string& upload_store) {
  int rtn_parse = OK;

  if (directive == "error_page") {
    rtn_parse = parseError_page(error_page, valueTokens);
  } else if (directive == "client_max_body_size") {
    rtn_parse = parseClient_max_body_size(client_max_body_size, valueTokens);
  } else if (directive == "root") {
    rtn_parse = parseRoot(root, valueTokens);
  } else if (directive == "auto_index") {
    rtn_parse = parseAuto_index(auto_index, valueTokens);
  } else if (directive == "index") {
    rtn_parse = parseIndex(index, valueTokens);
  } else if (directive == "index_if_dir") {
    rtn_parse = parseIndex(index_if_dir, valueTokens);
  } else if (directive == "allowed_method") {
    rtn_parse = parseAllowed_method(allowed_method, valueTokens);
  } else if (directive == "return") {
    rtn_parse = parseReturn_uri(return_uri, valueTokens);
  } else if (directive == "upload_store") {
    rtn_parse = parseUpload_store(upload_store, valueTokens);
  } else {
    return ERROR;
  }

  return rtn_parse;
}

int ConfigurationParser::parseHost(std::string& host,
                                   const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;
  if (!isHost(valueTokens[0])) return ERROR;

  host = valueTokens[0];

  return OK;
}

int ConfigurationParser::parsePort(int& port, const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;
  if (!isPort(valueTokens[0])) return ERROR;

  port = atoi(valueTokens[0].c_str());

  return OK;
}

int ConfigurationParser::parseServer_names(
    std::set<const std::string>& server_names, const Tokens& valueTokens) {
  if (valueTokens.size() < 1) return ERROR;

  for (size_t idx = 0; idx < valueTokens.size(); idx++) {
    server_names.insert(valueTokens[idx]);
  }

  return OK;
}

int ConfigurationParser::parseError_page(
    std::map<const int, const std::string>& error_page,
    const Tokens& valueTokens) {
  if (valueTokens.size() < 2) return ERROR;

  for (size_t idx_error_code = 0; idx_error_code < valueTokens.size() - 1;
       idx_error_code++) {
    if (!isErrorCode(valueTokens[idx_error_code])) return ERROR;

    error_page.insert(std::make_pair(atoi(valueTokens[idx_error_code].c_str()),
                                     valueTokens[valueTokens.size() - 1]));
  }

  return OK;
}

int ConfigurationParser::parseClient_max_body_size(
    std::string& client_max_body_size, const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  client_max_body_size = valueTokens[0];

  return OK;
}

int ConfigurationParser::parseRoot(std::string& root,
                                   const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  root = valueTokens[0];

  return OK;
}

int ConfigurationParser::parseAuto_index(bool& auto_index,
                                         const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;
  if (!isAutoIndex(valueTokens[0])) return ERROR;

  auto_index = (valueTokens[0] == "on" ? true : false);

  return OK;
}

int ConfigurationParser::parseIndex(std::string& index,
                                    const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  index = valueTokens[0];

  return OK;
}

int ConfigurationParser::parseIndex_if_dir(std::string& index_if_dir,
                                           const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  index_if_dir = valueTokens[0];

  return OK;
}

int ConfigurationParser::parseAllowed_method(
    std::set<const std::string>& allowed_method, const Tokens& valueTokens) {
  if (valueTokens.size() < 1) return ERROR;

  for (size_t idx_value = 0; idx_value < valueTokens.size(); idx_value++) {
    allowed_method.insert(valueTokens[idx_value]);
  }

  return OK;
}

int ConfigurationParser::parseReturn_uri(std::string& return_uri,
                                         const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  return_uri = valueTokens[0];

  return OK;
}

int ConfigurationParser::parseUpload_store(std::string& upload_store,
                                           const Tokens& valueTokens) {
  if (valueTokens.size() != 1) return ERROR;

  upload_store = valueTokens[0];

  return OK;
}

bool ConfigurationParser::isHost(const std::string& host) {
  const std::vector<const std::string> tmp = split(host, ".");

  if (tmp.size() != 4) return false;

  for (std::vector<const std::string>::const_iterator it = tmp.begin();
       it != tmp.end(); it++) {
    std::string octec = *it;
    int num = 0;

    for (size_t idx = 0; idx < octec.length(); idx++) {
      if (!std::isdigit(octec[idx])) return false;

      num = num * 10 + (octec[idx] - '0');
    }

    if (octec.length() > 5 || num > 255) return false;
  }

  return true;
}

bool ConfigurationParser::isPort(const std::string& port) {
  int num = 0;

  for (size_t idx = 0; idx < port.size(); idx++) {
    if (!std::isdigit(port[idx])) return false;

    num = num * 10 + (port[idx] - '0');
  }

  if (port.length() > 5 || num > 65535) return false;

  return true;
}

bool ConfigurationParser::isErrorCode(const std::string& error_code) {
  int num = 0;

  for (size_t idx = 0; idx < error_code.size(); idx++) {
    if (!std::isdigit(error_code[idx])) return false;

    num = num * 10 + (error_code[idx] - '0');
  }

  if (error_code.length() > 3 || num >= 600) return false;

  return true;
}

bool ConfigurationParser::isAutoIndex(const std::string& auto_index) {
  if (auto_index == "on" || auto_index == "off") return true;

  return false;
}
