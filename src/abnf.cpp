#include "abnf.hpp"

#include <cstdlib>

bool IsOctet(char c) {
  (void)static_cast<unsigned char>(c);
  return true;
}

bool IsHost(std::string s) {
  // host = IP-literal / IPv4address / reg-name
  size_t delimiter_pos;
  std::string sub_component;
  char* end_ptr;

  delimiter_pos = s.find(':');
  if (delimiter_pos != std::string::npos) {
    // port = *DIGIT
    sub_component = s.substr(delimiter_pos + 1);
    long port = std::strtol(sub_component.c_str(), &end_ptr, 10);
    if (end_ptr == sub_component || *end_ptr != 0 || port < 0 || port > 65535) {
      return false;
    }
    s = s.substr(0, delimiter_pos);
  }

  size_t length = s.length();
  if (length == 0) {
    /*
        A recipient that processes "http" URI with an empty host identifier
                reference MUST reject it as invalid.
        Section 4.2.1 of [HTTP]
    */
    return false;
  } else if (s[0] == '[') {
    // IP-literal = "[" ( IPv6address / IPvFuture ) "]"
    // 네트워크 정책 상 IPv6를 지원하지 않는다.
    return false;
  } else {
    /*
        IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
                        / reg-name = *( unreserved / pct-encoded / sub-delims )
    */
    for (size_t i = 0; i < length; ++i) {
      char c = s[i];
      if (c == '%' && !IsPctEncoded(s, i)) {
        return false;
      }
      if (!IsUnreserved(c) && !IsSubDlims(c)) {
        return false;
      }
    }

    return true;
  }
}

bool IsWhiteSpace(char c) {
  // WS = SP / HTAB
  if (c == 9 || c == 32) {
    return true;
  } else {
    return false;
  }
}

bool IsVchar(char c) {
  // VCHAR =  %x21-7E
  if (c < 33 || c > 126) {
    return false;
  } else {
    return true;
  }
}

bool IsObsText(unsigned char c) {
  // obs-text = %x80-FF
  if (c > 127) {
    return true;
  } else {
    return false;
  }
}

bool IsToken(const std::string s, std::string delimiter) {
  // token = 1*tchar
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    /*
                        tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                                / "+" / "-" / "." / "^" / "_" / "`" / "|" /
       "~" / DIGIT / ALPHA ; any VCHAR, except delimiters
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
        break;
      default:
        if (std::isalnum(c)) {
          break;
        } else if (delimiter.find(c) != delimiter.npos) {
          break;
        } else {
          return false;
        }
    }
  }
  return true;
}

bool IsUnreserved(char c) {
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

bool IsSubDlims(char c) {
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

bool IsPctEncoded(const std::string s, const size_t pos) {
  if (pos + 2 >= s.length() || !std::isxdigit(s[pos + 1]) ||
      !std::isxdigit(s[pos + 2])) {
    return false;
  }
  return true;
}
