#include <iostream>
#include <set>
#include <vector>

#include "configuration.hpp"
#include "configuration_parser.hpp"
#include "core.hpp"
#include "file_reader.hpp"
#include "multiplexer.hpp"

#define ARG_ERROR_MESSAGE "please check argument"

int parseConfig(const char* fileName, Configuration& configuration);

int main(const int argc, const char* argv[]) {
  Configuration configuration;

  if (argc == 1) {
    if (parseConfig("conf/default.conf", configuration) == ERROR) return ERROR;
  } else if (argc == 2) {
    if (parseConfig(argv[1], configuration) == ERROR) return ERROR;
  } else {
    std::cerr << ARG_ERROR_MESSAGE << std::endl;
    return ERROR;
  }

  Multiplexer multiplexer;

  if (multiplexer.Init(configuration) == ERROR) return ERROR;
  return multiplexer.Multiplexing();
}

int parseConfig(const char* fileName, Configuration& configuration) {
  std::string contents;

  if (FileReader::ReadFile(fileName, contents) == ERROR) {
    return ERROR;
  };

  if (ConfigurationParser::Parse(contents, configuration) == ERROR) {
    return ERROR;
  }

  return OK;
}
