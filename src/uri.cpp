#include "uri.hpp"

#include "abnf.hpp"
#include "utils.hpp"
/*
        uri.cpp에 사용되는 비멤버 함수
*/

int ParseSubComponent(Uri& uri, int (Uri::*Parser)(std::string& param),
                      std::string& uri_component, std::string delimiter);

int ParseSubComponent(Uri& uri, int (Uri::*Parser)(std::string& param),
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
    return (uri.*Parser)(sub_component);
  }
  return HTTP_OK;
}

/*
        URI 클래스 멤버 함수
*/
Uri::Uri()
    : request_target_form_(kAbsoluteForm),
      request_target_(""),
      decoded_request_target_(""),
      scheme_("http"),
      user_(""),
      password_(""),
      host_("localhost:80"),
      path_("/"),
      query_string_("") {}
Uri::~Uri() {}

Uri::Uri(const Uri& obj) { *this = obj; }
Uri& Uri::operator=(const Uri& obj) {
  if (this != &obj) {
    this->request_target_form_ = obj.request_target_form_;
    this->request_target_ = obj.request_target_;
    this->decoded_request_target_ = obj.decoded_request_target_;
    this->scheme_ = obj.scheme_;
    this->user_ = obj.user_;
    this->password_ = obj.password_;
    this->host_ = obj.host_;
    this->path_ = obj.path_;
    this->query_string_ = obj.query_string_;
  }
  return *this;
}

Uri::RequestTargetFrom Uri::request_target_form() const {
  return this->request_target_form_;
}
const std::string& Uri::request_target() const { return this->request_target_; }
const std::string& Uri::decoded_request_target() const {
  return this->decoded_request_target_;
}
const std::string& Uri::scheme() const { return this->scheme_; }
const std::string& Uri::user() const { return this->user_; }
const std::string& Uri::password() const { return this->password_; }
const std::string& Uri::host() const { return this->host_; }
const std::string& Uri::path() const { return this->path_; }
const std::string& Uri::query_string() const { return this->query_string_; }

int Uri::ParseScheme(std::string& scheme) {
  // Section 3.1 of [URI]

  scheme = ToCaseInsensitive(scheme);
  if (std::isalpha(scheme[0])) {
    int i = 0;
    while (scheme[++i]) {
      char c = scheme[i];
      if (std::isalnum(c) || c == '+' || c == '-' || c == '.') {
        continue;
      } else {
        return HTTP_BAD_REQUEST;
      }
    }
    this->scheme_ = scheme;
    return HTTP_OK;
  } else {
    return HTTP_BAD_REQUEST;
  }
}

int Uri::ParseAuthority(std::string& authority) {
  // Section 3.2 of [URI]
  size_t delimiter_pos;

  delimiter_pos = authority.find('@');
  if (delimiter_pos != std::string::npos) {
    // 보안 정책 상 userinfo를 포함한 요청을 모두 거부한다.
    return HTTP_BAD_REQUEST;
  }

  if (!IsHost(authority)) {
    return HTTP_BAD_REQUEST;
  }
  this->host_ = authority;
  return HTTP_OK;
}

int Uri::ParsePathSegment(std::string& path_segment) {
  /*
        path-abempty = *( "/" segment )

        segment = *pchar
        pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
  */

  size_t path_segment_length = path_segment.length();

  for (size_t i = 0; i < path_segment_length; ++i) {
    char c = path_segment[i];

    switch (c) {
      case '%':
        if (!IsPctEncoded(path_segment, i)) {
          return HTTP_BAD_REQUEST;
        }
      case '/':
      case ';':
      case '=':
      case ':':
      case '@':
        break;
      default:
        if (!IsUnreserved(c) && !IsSubDlims(c)) {
          return HTTP_BAD_REQUEST;
        }
        break;
    }
  }
  this->path_ = path_segment;
  return HTTP_OK;
}

int Uri::ParseQuery(std::string& query) {
  // query = *( pchar / "/" / "?" )
  // pchar = unreserved / pct-encoded / sub-delims / ":" / "@"

  size_t query_length = query.length();

  this->query_string_ = query;
  for (size_t i = 0; i < query_length; ++i) {
    char c = query[i];
    switch (c) {
      case '%':
        if (!IsPctEncoded(query, i)) {
          return HTTP_BAD_REQUEST;
        }
      case '&':
      case '=':
      case ':':
      case '@':
      case '/':
      case '?':
        break;
      default:
        if (!IsUnreserved(c) && !IsSubDlims(c)) {
          return HTTP_BAD_REQUEST;
        }
        break;
    }
  }
  return HTTP_OK;
}

int Uri::ParseUriComponent(std::string request_uri) {
  /*
                absolute-form = absolute-URI
                absolute-URI = scheme ":" hier-part [ "?" query ]
                hier-part = "//" authority path-abempty
                path-abempty = *( "/" segment )

                origin-form = absolute-path [ "?" query ]
                absolute-path = 1*( "/" segment )
   */

  // todo - request_uri 존재 여부는 상위 레벨에서 처리되어야 할 것 같다.`
  if (request_uri.length() == 0) {
    return HTTP_BAD_REQUEST;
  }

  if (request_uri.length() > 8000) {
    return HTTP_REQUEST_URI_TOO_LARGE;
  }

  if (ParseSubComponent(*this, &Uri::ParseQuery, request_uri, "?") ==
      HTTP_BAD_REQUEST) {
    return HTTP_BAD_REQUEST;
  }

  if (request_uri[0] != '/') {
    // absolute-form
    request_target_form_ = kAbsoluteForm;
    if (ParseSubComponent(*this, &Uri::ParseScheme, request_uri, "://") ==
            HTTP_BAD_REQUEST ||
        ParseSubComponent(*this, &Uri::ParseAuthority, request_uri, "/") ==
            HTTP_BAD_REQUEST ||
        this->scheme_ == "" || this->host_ == "") {
      return HTTP_BAD_REQUEST;
    }
  } else {
    // origin-form
    request_target_form_ = kOriginForm;
  }

  if (request_uri.size() == 0 ||
      ParsePathSegment(request_uri) == HTTP_BAD_REQUEST) {
    return HTTP_BAD_REQUEST;
  }

  return HTTP_OK;
}

std::string Uri::DecodeRequestTarget() {
  std::stringstream decode;
  size_t i = 0;
  size_t length = request_target_.length();
  while (i < length) {
    char c = request_target_[i];
    if (c == '%') {
      std::string hex = request_target_.substr(i + 1, i + 3);
      char octet = static_cast<char>(std::strtol(hex.c_str(), NULL, 16));
      decode << octet;
      i += 3;
    } else {
      decode << c;
      i += 1;
    }
  }
  decoded_request_target_ = decode.str();
  return decoded_request_target_;
}

int Uri::ReconstructTargetUri(std::string request_host) {
  if (this->request_target_form_ == kOriginForm) {
    this->scheme_ = "http";
    this->host_ = request_host;
  }
  this->request_target_ = this->path_;
  DecodeRequestTarget();
  return HTTP_OK;
}
