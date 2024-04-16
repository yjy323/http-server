#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <unistd.h>

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

  eType GetType(std::string path);
  std::string GetContents(std::string path);
  std::string GetExtension(std::string path);
  std::string GetMimeType(std::string extension);

  void ReadFile();
  void ReadBuffer();

  std::string path() const;
  eType type();
  std::string body() const;
  std::string extension() const;
  std::string mime_type() const;
  ssize_t length_n();

 private:
  std::string path_;
  eType type_;
  std::string body_;
  std::string extension_;
  std::string mime_type_;
  ssize_t length_n_;
};

#endif
