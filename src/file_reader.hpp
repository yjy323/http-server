#ifndef FILE_READER_HPP
#define FILE_READER_HPP

#include <fstream>
#include <string>

#include "core.hpp"

class FileReader {
 public:
  virtual ~FileReader();

  static int ReadFile(const std::string& filePath, std::string& contents);

 private:
  FileReader();
  FileReader(const FileReader& ref);

  FileReader& operator=(const FileReader& ref);
};

#endif
