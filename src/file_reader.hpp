#ifndef FILE_READER_HPP
#define FILE_READER_HPP

#include <fstream>
#include <string>

#include "core.hpp"

class FileReader {
 private:
  FileReader();
  FileReader(const FileReader& ref);

  FileReader& operator=(const FileReader& ref);

 public:
  virtual ~FileReader();

  static Return readFile(const std::string& filePath, std::string& contents);
};

#endif
