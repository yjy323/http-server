#include "url.hpp"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

// 임시
#define OK 0
#define ERROR 1

/*
 Todo: 생성자 구체화, 초기화에 서버 정보 필요, ABNF 룰 함수 분리
*/
Url::Url() {}
Url::Url(const Url& obj) { (void)obj; }
Url::~Url() {}
Url& Url::operator=(const Url& obj) {
  (void)obj;
  return *this;
}

void ToCaseInsensitve(std::string& str) {
  for (size_t i = 0; i < str.length(); ++i) {
    str[i] = std::tolower(str[i]);
  }
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

int Url::ParseScheme(std::string& scheme) {
  // Section 3.1 of [URI]

  ToCaseInsensitve(scheme);
  if (std::isalpha(scheme[0])) {
    int i = 0;
    while (scheme[++i]) {
      char c = scheme[i];
      if (std::isalnum(c) || c == '+' || c == '-' || c == '.') {
        continue;
      } else {
        return ERROR;
      }
    }
    return OK;
  }
  return ERROR;
}

int Url::ParseAuthority(std::string& authority) {
  // Section 3.2 of [URI]
  size_t delimiter_pos;
  std::string sub_component;
  char* end_ptr;

  delimiter_pos = authority.find('@');
  if (delimiter_pos != std::string::npos) {
    // 보안 정책 상 userinfo를 포함한 요청을 모두 거부한다.
    return ERROR;
  }

  delimiter_pos = authority.find(':');
  if (delimiter_pos != std::string::npos) {
    // port = *DIGIT
    sub_component = authority.substr(delimiter_pos + 1);
    authority = authority.substr(0, delimiter_pos);

    long port = strtol(sub_component.c_str(), &end_ptr, 10);
    if (end_ptr == sub_component || *end_ptr != 0 || port < 0 || port > 65535) {
      return ERROR;
    }
    this->port_ = port;
  }

  // host = IP-literal / IPv4address / reg-name
  size_t authority_length = authority.length();
  if (authority_length == 0) {
    /*
        A recipient that processes "http" URI with an empty host identifier
                reference MUST reject it as invalid.
        Section 4.2.1 of [HTTP]
    */
    return ERROR;
  } else if (authority[0] == '[') {
    // IP-literal = "[" ( IPv6address / IPvFuture ) "]"
    // 네트워크 정책 상 IPv6를 지원하지 않는다.
    return ERROR;

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
    for (int i = 0; i < 4; i++) {
      delimiter_pos = authority.find(".", pre_delimiter_pos);
      sub_component = authority.substr(pre_delimiter_pos, delimiter_pos);
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
      for (size_t i = 0; i < authority_length; i++) {
        char c = authority[i];
        if (IsUnreserved(c) || IsSubDlims(c)) {
          continue;
        } else if (c == '%') {
          // pct-encoded = "%" HEXDIG HEXDIG
          for (int j = 1; j < 3; j++) {
            if (i + j == authority_length || !std::isxdigit(authority[i + j])) {
              return ERROR;
            }
          }
        }
      }
    }

    this->host_ = authority;
    return OK;
  }
}

// int Url::ParsePathComponent(std::string& path_component) {}

std::vector<std::string> Split(std::string& str, char delimiter) {
  std::istringstream iss(str);
  std::string buffer;
  std::vector<std::string> buffer_container;

  while (getline(iss, buffer, delimiter)) {
    buffer_container.push_back(buffer);
  }
  return buffer_container;
}
int Url::ParseUriComponent(std::string& request_uri) {
  // https://www.example.org/pub/WWW/TheProject.html
  // scheme "://" authority *(/path;key=value)[? key = value]  #fragment
  /*
        1. origin form VS absolute from
        2. parse by specific form
        3. origin form
            - split: path-component
        4. absolute form
            - split: scheme "://" authority ...
        5. validation
            - scheme, host, port , path, parameter ... 각자
  */
  return ParseAuthority(request_uri);
}
