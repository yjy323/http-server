#include "abnf.hpp"

bool Abnf::IsOctet(char c) { return (static_cast<unsigned char>(c) <= 255); }

bool Abnf::IsHost(std::string& s) {
  // host = IP-literal / IPv4address / reg-name
  size_t delimiter_pos;
  std::string sub_component;
  char* end_ptr;

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
    // IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
    /*
            dec-octet = DIGIT               ; 0-9
                        / %x31-39 DIGIT     ; 10-99
                        / "1" 2DIGIT        ; 100-199
                        / "2" %x30-34 DIGIT ; 200-249
                        / "25" %x30-35      ; 250-255
    */

    bool is_ip_v4_address = true;
    size_t pre_delimiter_pos = 0;
    for (int i = 0; i < 4; ++i) {
      delimiter_pos = s.find(".", pre_delimiter_pos);
      sub_component = s.substr(pre_delimiter_pos, delimiter_pos);
      pre_delimiter_pos = delimiter_pos;
      long dec_octet = strtol(sub_component.c_str(), &end_ptr, 10);
      if (end_ptr == sub_component || *end_ptr != 0 || dec_octet < 0 ||
          dec_octet > 255) {
        is_ip_v4_address = false;
        break;
      }
    }

    if (is_ip_v4_address == false) {
      // reg-name = *( unreserved / pct-encoded / sub-delims )
      for (size_t i = 0; i < length; ++i) {
        char c = s[i];

        switch (c) {
          if (!Abnf::IsPctEncoded(s, i)) {
            return false;
          }
          default:
            if (!Abnf::IsUnreserved(c) && !Abnf::IsSubDlims(c)) {
              return false;
            }
            break;
        }
      }
    }

    return true;
  }
}

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

bool Abnf::IsPctEncoded(std::string& s, const size_t pos) {
  if (pos + 2 >= s.length() || !std::isxdigit(s[pos + 1]) ||
      !std::isxdigit(s[pos + 2])) {
    return false;
  }
  return true;
}
