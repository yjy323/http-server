#include "utils.hpp"

void ToCaseInsensitve(std::string& str) {
  for (size_t i = 0; i < str.length(); ++i) {
    str[i] = std::tolower(str[i]);
  }
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

const std::vector<const std::string> split(const std::string& str,
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
