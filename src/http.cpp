#include "http.hpp"

#include "abnf.hpp"
#include "utils.hpp"

const char* GetHttpStatus(int status_code) {
  if (status_code >= 100 && status_code <= 507) {
    switch (status_code) {
      case HTTP_CONTINUE:
        return HTTP_STATUS[0];
      case HTTP_SWITCHING_PROTOCOLS:
        return HTTP_STATUS[1];
      case HTTP_PROCESSING:
        return HTTP_STATUS[2];
      case HTTP_OK:
        return HTTP_STATUS[3];
      case HTTP_CREATED:
        return HTTP_STATUS[4];
      case HTTP_ACCEPTED:
        return HTTP_STATUS[5];
      case HTTP_NO_CONTENT:
        return HTTP_STATUS[6];
      case HTTP_PARTIAL_CONTENT:
        return HTTP_STATUS[7];
      case HTTP_SPECIAL_RESPONSE:
        return HTTP_STATUS[8];
      case HTTP_MOVED_PERMANENTLY:
        return HTTP_STATUS[9];
      case HTTP_MOVED_TEMPORARILY:
        return HTTP_STATUS[10];
      case HTTP_SEE_OTHER:
        return HTTP_STATUS[11];
      case HTTP_NOT_MODIFIED:
        return HTTP_STATUS[12];
      case HTTP_TEMPORARY_REDIRECT:
        return HTTP_STATUS[13];
      case HTTP_PERMANENT_REDIRECT:
        return HTTP_STATUS[14];
      case HTTP_BAD_REQUEST:
        return HTTP_STATUS[15];
      case HTTP_UNAUTHORIZED:
        return HTTP_STATUS[16];
      case HTTP_FORBIDDEN:
        return HTTP_STATUS[17];
      case HTTP_NOT_FOUND:
        return HTTP_STATUS[18];
      case HTTP_NOT_ALLOWED:
        return HTTP_STATUS[19];
      case HTTP_REQUEST_TIME_OUT:
        return HTTP_STATUS[20];
      case HTTP_CONFLICT:
        return HTTP_STATUS[21];
      case HTTP_LENGTH_REQUIRED:
        return HTTP_STATUS[22];
      case HTTP_PRECONDITION_FAILED:
        return HTTP_STATUS[23];
      case HTTP_REQUEST_ENTITY_TOO_LARGE:
        return HTTP_STATUS[24];
      case HTTP_REQUEST_URI_TOO_LARGE:
        return HTTP_STATUS[25];
      case HTTP_UNSUPPORTED_MEDIA_TYPE:
        return HTTP_STATUS[26];
      case HTTP_RANGE_NOT_SATISFIABLE:
        return HTTP_STATUS[27];
      case HTTP_MISDIRECTED_REQUEST:
        return HTTP_STATUS[28];
      case HTTP_TOO_MANY_REQUESTS:
        return HTTP_STATUS[29];
      case HTTP_INTERNAL_SERVER_ERROR:
        return HTTP_STATUS[30];
      case HTTP_NOT_IMPLEMENTED:
        return HTTP_STATUS[31];
      case HTTP_BAD_GATEWAY:
        return HTTP_STATUS[32];
      case HTTP_SERVICE_UNAVAILABLE:
        return HTTP_STATUS[33];
      case HTTP_GATEWAY_TIME_OUT:
        return HTTP_STATUS[34];
      case HTTP_VERSION_NOT_SUPPORTED:
        return HTTP_STATUS[35];
      case HTTP_INSUFFICIENT_STORAGE:
        return HTTP_STATUS[36];
      default:
        return "";  // 유효하지 않은 상태 코드
    }
  }
}

const std::string InsertHeader(std::map<HeaderKey, HeaderValue>& headers,
                               const std::string key, const std::string value) {
  if (value.size() == 0) {
    return;
  }
  HeadersConstIterator end = headers.end();
  HeadersIterator it = headers.find(key);
  if (it == end) {
    headers.insert(std::make_pair(key, HeaderValue()));
    it = headers.find(key);
  }
  it->second.push_back(value);
  return key;
}

int ProcessHttpHeaderHost(HeadersIn& headers, std::string value) {
  if (value.size() == 0 || !IsHost(value)) {
    return HTTP_BAD_REQUEST;
  } else if (headers.host_ != "") {
    return HTTP_BAD_REQUEST;
  } else {
    headers.host_ = value;
    return HTTP_OK;
  }
}
int ProcessHttpHeaderTransferEncoding(HeadersIn& headers, std::string value) {
  if (!IsToken(value)) {
    return HTTP_BAD_REQUEST;
  } else if (ToCaseInsensitive(Trim(value)) != "chunked") {
    return HTTP_NOT_IMPLEMENTED;
  } else {
    headers.transfer_encoding_ = value;
    headers.chuncked = true;
    return HTTP_OK;
  }
}

int ProcessHttpHeaderContentLength(HeadersIn& headers, std::string value) {
  char* end_ptr;

  ssize_t content_length_n = strtol(value.c_str(), &end_ptr, 10);
  if (end_ptr == value || *end_ptr != 0) {
    return HTTP_BAD_REQUEST;
  } else {
    headers.content_length_ = value;
    headers.content_length_n_ = content_length_n;
    return HTTP_OK;
  }
}
