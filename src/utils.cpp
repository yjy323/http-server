#include "utils.hpp"

void ToCaseInsensitve(std::string& str) {
  for (size_t i = 0; i < str.length(); ++i) {
    str[i] = std::tolower(str[i]);
  }
}

std::string Trim(const std::string s) {
  size_t front = 0;
  size_t back = s.npos;

  for (; front < s.length(); ++front) {
    if (s[front] != 9 && s[front] != 32) {
      break;
    }
  }

  for (; back >= 0; --back) {
    if (s[back] != 9 && s[back] != 32) {
      break;
    }
  }

  return s.substr(front, back);
}

std::vector<std::string> Split(std::string& str, const char delimiter) {
  std::istringstream iss(str);
  std::string buffer;
  std::vector<std::string> buffer_container;

  while (getline(iss, buffer, delimiter)) {
    buffer_container.push_back(buffer);
  }
  return buffer_container;
}

const std::vector<const std::string> Split(const std::string& str,
                                           const char delimiter) {
  std::istringstream iss(str);
  std::string buffer;
  std::vector<const std::string> buffer_container;

  while (getline(iss, buffer, delimiter)) {
    buffer_container.push_back(buffer);
  }
  return buffer_container;
}

std::vector<std::string> Split(std::string& str, const std::string& sep) {
  std::vector<std::string> res;

  size_t idxStart = 0;
  for (size_t idx = 0; idx < str.length(); idx++) {
    if (str.substr(idx, sep.length()).compare(sep) == 0) {
      res.push_back(str.substr(idxStart, idx - idxStart));
      idxStart = idx + sep.length();
      idx += sep.length() - 1;
    }
  }

  if (idxStart != str.length())
    res.push_back(str.substr(idxStart, str.length() - idxStart));

  return res;
}

const std::vector<const std::string> Split(const std::string& str,
                                           const std::string& sep) {
  std::vector<const std::string> res;

  size_t idxStart = 0;
  for (size_t idx = 0; idx < str.length(); idx++) {
    if (str.substr(idx, sep.length()).compare(sep) == 0) {
      res.push_back(str.substr(idxStart, idx - idxStart));
      idxStart = idx + sep.length();
      idx += sep.length() - 1;
    }
  }

  if (idxStart != str.length())
    res.push_back(str.substr(idxStart, str.length() - idxStart));

  return res;
}
