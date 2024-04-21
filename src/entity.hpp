#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
class Entity {
 public:
  enum eType { kFile, kDirectory };

  Entity();
  Entity(std::string path);
  Entity(const Entity& obj);
  ~Entity();
  Entity& operator=(const Entity& obj);

  static bool IsFileExist(const char* path);
  static bool IsFileReadable(const char* path);
  static bool IsFileExecutable(const char* path);

  std::string CreatePage(std::string body_line);
  std::string CreateDirectoryListingPage(std::string path,
                                         std::string request_target);
  std::string ReadFile(const char* path);

  std::string path() const;
  eType type();
  std::string body() const;
  std::string extension() const;
  std::string mime_type() const;
  std::string modified_s() const;
  std::time_t modified_t() const;
  std::string length() const;
  ssize_t length_n();

 private:
  eType GetType(std::string path);
  std::string GetContents(std::string path);
  std::string GetExtension(std::string path);
  std::string GetMimeType(std::string extension);
  std::time_t GetModifiedTime(std::string path);

  std::string path_;
  eType type_;
  std::string body_;
  std::string extension_;
  std::string mime_type_;
  std::string modified_s_;
  std::time_t modified_t_;
  std::string length_;
  ssize_t length_n_;
};

#endif
