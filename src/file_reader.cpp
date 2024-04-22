#include "file_reader.hpp"

#include <iostream>
#include <sstream>

#define ERROR_MESSAGE "can not open the configuration file"

FileReader::~FileReader() {}

int FileReader::ReadFile(const std::string& filePath, std::string& contents) {
  std::ifstream fileStream(filePath.c_str());
  std::ostringstream buffer;

  if (!fileStream.is_open()) {
    return ERROR;
  };

  buffer << fileStream.rdbuf();

  contents = buffer.str();

  return OK;
}
