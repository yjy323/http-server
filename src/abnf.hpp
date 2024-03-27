#ifndef ABNF_HPP
#define ABNF_HPP

#include <cctype>
#include <iostream>

class Abnf {
 private:
 public:
  static bool IsToken(const std::string& s);
  static bool IsUnreserved(const char c);
  static bool IsSubDlims(const char c);
  static char DecodePctEncoded(std::string& s, const size_t pos);
};

#endif
