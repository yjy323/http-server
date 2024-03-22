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
