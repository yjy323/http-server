#include "url.hpp"

// 임시
#define OK 0
#define ERROR 1

/*
        URL 클래스의 비멤버 함수
*/
static void InsertKeyValuePair(std::map<const std::string, std::string>& map,
                               const std::string& key, std::string value);

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
    for (int i = 0; i < 4; ++i) {
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
      for (size_t i = 0; i < authority_length; ++i) {
        char c = authority[i];

        switch (c) {
          case '%':
            c = Abnf::DecodePctEncoded(authority, i);
            if (c == -1) {
              return ERROR;
            }
            break;
          default:
            if (!Abnf::IsUnreserved(c) && !Abnf::IsSubDlims(c)) {
              return ERROR;
            }
            break;
        }
      }
    }

    this->host_ = authority;
    return OK;
  }
}

void InsertKeyValuePair(std::map<const std::string, std::string>& map,
                        const std::string& key, std::string value) {
  if (value != "") {
    map[key] = value;
  } else {
    map.insert(std::make_pair(key, value));
  }
}

int Url::ParsePathSegment(std::string& path_segment) {
  /*
        path-abempty = *( "/" segment )

        segment = *pchar
        pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
  */
  PathSegment path_segment_obj;
  std::string key;

  std::stringstream ss;
  bool is_before_param = true;

  size_t path_segment_length = path_segment.length();

  for (size_t i = 0; i < path_segment_length; ++i) {
    char c = path_segment[i];

    switch (c) {
      case '%':
        c = Abnf::DecodePctEncoded(path_segment, i);
        if (c == -1) {
          return ERROR;
        }
        break;
      case ';':
        if (is_before_param) {
          path_segment_obj.path = ss.str();
        } else if (key != "") {
          InsertKeyValuePair(path_segment_obj.parameter, key, ss.str());
        }
        is_before_param = false;
        key = "";
        ss.str(std::string());
        break;
      case '=':
        key = ss.str();
        ss.str(std::string());
        break;
      case ':':
      case '@':
        break;
      default:
        if (!Abnf::IsUnreserved(c) && !Abnf::IsSubDlims(c)) {
          return ERROR;
        } else {
          ss << c;
        }
        break;
    }
  }
  if (is_before_param) {
    path_segment_obj.path = ss.str();
  } else if (key != "") {
    InsertKeyValuePair(path_segment_obj.parameter, key, ss.str());
  }
  this->path_segments_.push_back(path_segment_obj);

  return OK;
}

int Url::ParseQuery(std::string& query) {
  // query = *( pchar / "/" / "?" )
  // pchar = unreserved / pct-encoded / sub-delims / ":" / "@"

  std::string key;

  std::stringstream ss;

  size_t query_length = query.length();

  for (size_t i = 0; i < query_length; ++i) {
    char c = query[i];

    switch (c) {
      case '%':
        c = Abnf::DecodePctEncoded(query, i);
        if (c == -1) {
          return ERROR;
        }
        break;
      case '&':
        if (key != "") {
          InsertKeyValuePair(query_, key, ss.str());
        }
        key = "";
        ss.str(std::string());
        break;
      case '=':
        key = ss.str();
        ss.str(std::string());
        break;
      case ':':
      case '@':
      case '/':
      case '?':
        break;
      default:
        if (!Abnf::IsUnreserved(c) && !Abnf::IsSubDlims(c)) {
          return ERROR;
        } else {
          ss << c;
        }
        break;
    }
  }

  if (key != "") {
    InsertKeyValuePair(query_, key, ss.str());
  }
  return OK;
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
  (void)request_uri;
  return OK;
}
