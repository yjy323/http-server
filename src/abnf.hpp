#ifndef ABNF_HPP
#define ABNF_HPP

#include <cctype>

class Abnf {
 private:
 public:
  static bool IsUnreserved(char c);
  static bool IsSubDlims(char c);
};

#endif
