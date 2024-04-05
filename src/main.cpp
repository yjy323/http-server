#include <iostream>
#include <set>
#include <vector>

#include "configuration.hpp"
#include "configuration_parser.hpp"
#include "core.hpp"
#include "file_reader.hpp"
#include "multiplexing.hpp"
#include "socket.hpp"

int parseConfig(const char* fileName, Configuration& configuration);

int main(const int argc, const char* argv[]) {
  Configuration configuration;

  if (argc == 1) {
    if (parseConfig("conf/default.conf", configuration) == ERROR) return ERROR;
  } else if (argc == 2) {
    if (parseConfig(argv[1], configuration) == ERROR) return ERROR;
  } else {
    std::cerr << "Error: argument error" << std::endl;
    return ERROR;
  }

  return Multiplexing(configuration);
}

int parseConfig(const char* fileName, Configuration& configuration) {
  std::string contents;

  if (FileReader::readFile(fileName, contents) == ERROR) {
    std::cerr << "Error: can not open the configuration file" << std::endl;
    return ERROR;
  };

  if (ConfigurationParser::parse(contents, configuration) == ERROR) {
    std::cerr << "Error: can not parse the configuration file" << std::endl;
    return ERROR;
  }

  return OK;
}
