#include "url.hpp"

// 임시
#define OK 0
#define ERROR 1

/*
        url.cpp에 사용되는 비멤버 함수
*/
static void InsertKeyValuePair(std::map<const std::string, std::string>& map,
                               const std::string& key, std::string value);

static int ParseSubComponent(Url& url, int (Url::*Parser)(std::string& param),
                             std::string& uri_component, std::string delimiter);

void InsertKeyValuePair(std::map<const std::string, std::string>& map,
                        const std::string& key, std::string value) {
  if (value != "") {
    map[key] = value;
  } else {
    map.insert(std::make_pair(key, value));
  }
}

int ParseSubComponent(Url& url, int (Url::*Parser)(std::string& param),
                      std::string& uri_component, std::string delimiter) {
  size_t delimiter_pos;
  std::string sub_component;

  delimiter_pos = uri_component.find(delimiter);
  if (delimiter_pos != std::string::npos) {
    if (delimiter == "?") {
      // additional component
      sub_component = uri_component.substr(delimiter_pos + delimiter.length());
      uri_component = uri_component.substr(0, delimiter_pos);
    } else {
      sub_component = uri_component.substr(0, delimiter_pos);
      uri_component = uri_component.substr(delimiter_pos + delimiter.length());
    }
    return (url.*Parser)(sub_component);
  }
  return OK;
}

/*
        URL 클래스 멤버 함수
*/
Url::Url() {}
Url::~Url() {}

Url::Url(const Url& obj) { *this = obj; }
Url& Url::operator=(const Url& obj) {
  if (this != &obj) {
    this->scheme_ = obj.scheme_;
    this->host_ = obj.host_;
    this->port_ = obj.port_;
    this->path_segments_ = obj.path_segments_;
    this->query_ = obj.query_;
  }
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
    this->scheme_ = scheme;
    return OK;
  } else {
    return ERROR;
  }
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
    long port = strtol(sub_component.c_str(), &end_ptr, 10);
    if (end_ptr == sub_component || *end_ptr != 0 || port < 0 || port > 65535) {
      return ERROR;
    }
    this->port_ = port;

    authority = authority.substr(0, delimiter_pos);
  }

  if (!Abnf::IsHost(authority)) {
    return ERROR;
  }

  this->host_ = authority;
  return OK;
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
        path_segment_length = path_segment.length();
        if (c < 0) {
          return ERROR;
        }
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
        query_length = query.length();
        if (c < 0) {
          return ERROR;
        }
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
        ss << c;
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
  /*
                absolute-form = absolute-URI
                absolute-URI = scheme ":" hier-part [ "?" query ]
                hier-part = "//" authority path-abempty
                path-abempty = *( "/" segment )

                origin-form = absolute-path [ "?" query ]
                absolute-path = 1*( "/" segment )
   */

  typedef std::vector<std::string>::iterator pc_iterator;

  // todo - request_uri 존재 여부는 상위 레벨에서 처리되어야 할 것 같다.
  if (request_uri.length() == 0) {
    return ERROR;
  }

  if (ParseSubComponent(*this, &Url::ParseQuery, request_uri, "?") == ERROR) {
    return ERROR;
  }

  if (request_uri[0] != '/') {
    // absolute-form
    request_target_form_ = kAbsoluteForm;
    if (ParseSubComponent(*this, &Url::ParseScheme, request_uri, "://") ==
            ERROR ||
        ParseSubComponent(*this, &Url::ParseAuthority, request_uri, "/") ==
            ERROR ||
        this->scheme_ == "" || this->host_ == "") {
      return ERROR;
    }
  } else {
    // origin-form
    request_target_form_ = kOriginForm;
    request_uri.erase(0, 1);
  }

  std::vector<std::string> path_component = Split(request_uri, '/');
  for (pc_iterator pit = path_component.begin(); pit != path_component.end();
       ++pit) {
    if (ParsePathSegment(*pit) == ERROR) {
      return ERROR;
    }
  }
  if (this->path_segments_.size() == 0) {
    return ERROR;
  }

  return OK;
}

const std::string& Url::getScheme() const { return this->scheme_; }

const std::string& Url::getUser() const { return this->user_; }

const std::string& Url::getPassword() const { return this->password_; }

const std::string& Url::getHost() const { return this->host_; }

int Url::getPort() const { return this->port_; }

const std::list<Url::PathSegment>& Url::getPathSegments() const {
  return this->path_segments_;
}

const std::map<const std::string, std::string>& Url::getQuery() const {
  return this->query_;
}
