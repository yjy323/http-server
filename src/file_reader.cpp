#include "file_reader.hpp"

#include <sstream>

FileReader::~FileReader() {}

int FileReader::readFile(const std::string& filePath, std::string& contents) {
  std::ifstream fileStream(filePath.c_str());
  std::ostringstream buffer;

  if (!fileStream.is_open()) {
    return ERROR;
  };

  buffer << fileStream.rdbuf();

  contents = buffer.str();

  return OK;
}
