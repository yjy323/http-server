#include "utils.hpp"

const std::vector<const std::string> split(const std::string &str,
                                           const std::string &sep) {
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

void *Memset(void *b, int c, const size_t len) {
  unsigned char *bb;
  unsigned char cc;
  size_t i;

  bb = (unsigned char *)b;
  cc = (unsigned char)c;
  i = 0;

  for (i = 0; i < len; i++) {
    bb[i] = cc;
  }

  return ((void *)b);
}