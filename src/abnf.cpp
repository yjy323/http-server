#include "abnf.hpp"

bool Abnf::IsUnreserved(char c) {
  // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
  if (std::isalnum(c)) {
    return true;
  } else {
    switch (c) {
      case '-':
      case '.':
      case '_':
      case '~':
        return true;
      default:
        return false;
    }
  }
}

bool Abnf::IsSubDlims(char c) {
  /*
        sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / ","
                / ";" / "="
  */
  switch (c) {
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case ';':
    case '=':
      return true;
    default:
      return false;
  }
}

char Abnf::DecodePctEncoded(std::string& s, const size_t pos) {
  if (pos + 2 >= s.length() || !std::isxdigit(s[pos + 1]) ||
      !std::isxdigit(s[pos + 2])) {
    return -1;
  } else {
    char c = strtol(s.substr(pos + 1, pos + 3).c_str(), NULL, 16);
    s.replace(pos, pos + 2, std::string(1, c));
    return c;
  }
}
