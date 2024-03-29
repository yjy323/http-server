#include "abnf.hpp"

bool Abnf::IsWhiteSpace(char c) {
  // WS = SP / HTAB
  if (c == 9 || c == 32) {
    return true;
  } else {
    return false;
  }
}

bool Abnf::IsVchar(char c) {
  // VCHAR =  %x21-7E
  if (c < 33 || c > 126) {
    return false;
  } else {
    return true;
  }
}

bool Abnf::IsObsText(unsigned char c) {
  // obs-text = %x80-FF
  if (c < 128 || c > 255) {
    return false;
  } else {
    return true;
  }
}

bool Abnf::IsToken(const std::string& s) {
  // token = 1*tchar
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];

    /*
                        tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                                / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                                / DIGIT / ALPHA
                                ; any VCHAR, except delimiters
    */
    switch (c) {
      case '!':
      case '#':
      case '$':
      case '%':
      case '&':
      case '\'':
      case '*':
      case '+':
      case '-':
      case '.':
      case '^':
      case '_':
      case '`':
      case '|':
      case '~':
        return true;
      default:
        if (std::isalnum(c)) {
          return true;
        } else {
          return false;
        }
    }
  }
  return true;
}

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
