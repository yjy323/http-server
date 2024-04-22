#include "http.hpp"

#include "abnf.hpp"
#include "utils.hpp"

const char* HttpGetReasonPhase(int status_code) {
  switch (status_code) {
    case HTTP_CONTINUE:
      return REASON_PHASE[0];
    case HTTP_SWITCHING_PROTOCOLS:
      return REASON_PHASE[1];
    case HTTP_PROCESSING:
      return REASON_PHASE[2];
    case HTTP_OK:
      return REASON_PHASE[3];
    case HTTP_CREATED:
      return REASON_PHASE[4];
    case HTTP_ACCEPTED:
      return REASON_PHASE[5];
    case HTTP_NO_CONTENT:
      return REASON_PHASE[6];
    case HTTP_PARTIAL_CONTENT:
      return REASON_PHASE[7];
    case HTTP_SPECIAL_RESPONSE:
      return REASON_PHASE[8];
    case HTTP_MOVED_PERMANENTLY:
      return REASON_PHASE[9];
    case HTTP_MOVED_TEMPORARILY:
      return REASON_PHASE[10];
    case HTTP_SEE_OTHER:
      return REASON_PHASE[11];
    case HTTP_NOT_MODIFIED:
      return REASON_PHASE[12];
    case HTTP_TEMPORARY_REDIRECT:
      return REASON_PHASE[13];
    case HTTP_PERMANENT_REDIRECT:
      return REASON_PHASE[14];
    case HTTP_BAD_REQUEST:
      return REASON_PHASE[15];
    case HTTP_UNAUTHORIZED:
      return REASON_PHASE[16];
    case HTTP_FORBIDDEN:
      return REASON_PHASE[17];
    case HTTP_NOT_FOUND:
      return REASON_PHASE[18];
    case HTTP_NOT_ALLOWED:
      return REASON_PHASE[19];
    case HTTP_REQUEST_TIME_OUT:
      return REASON_PHASE[20];
    case HTTP_CONFLICT:
      return REASON_PHASE[21];
    case HTTP_LENGTH_REQUIRED:
      return REASON_PHASE[22];
    case HTTP_PRECONDITION_FAILED:
      return REASON_PHASE[23];
    case HTTP_REQUEST_ENTITY_TOO_LARGE:
      return REASON_PHASE[24];
    case HTTP_REQUEST_URI_TOO_LARGE:
      return REASON_PHASE[25];
    case HTTP_UNSUPPORTED_MEDIA_TYPE:
      return REASON_PHASE[26];
    case HTTP_RANGE_NOT_SATISFIABLE:
      return REASON_PHASE[27];
    case HTTP_MISDIRECTED_REQUEST:
      return REASON_PHASE[28];
    case HTTP_TOO_MANY_REQUESTS:
      return REASON_PHASE[29];
    case HTTP_INTERNAL_SERVER_ERROR:
      return REASON_PHASE[30];
    case HTTP_NOT_IMPLEMENTED:
      return REASON_PHASE[31];
    case HTTP_BAD_GATEWAY:
      return REASON_PHASE[32];
    case HTTP_SERVICE_UNAVAILABLE:
      return REASON_PHASE[33];
    case HTTP_GATEWAY_TIME_OUT:
      return REASON_PHASE[34];
    case HTTP_VERSION_NOT_SUPPORTED:
      return REASON_PHASE[35];
    case HTTP_INSUFFICIENT_STORAGE:
      return REASON_PHASE[36];
    default:
      return "";  // 유효하지 않은 상태 코드
  }
}

const std::string HttpInsertHeader(std::map<HeaderKey, HeaderValue>& headers,
                                   const std::string key,
                                   const std::string value) {
  if (value.size() == 0) {
    return "";
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

int HttpHost(HeadersIn& headers_in, std::string value) {
  if (value.size() == 0 || !IsHost(value)) {
    return HTTP_BAD_REQUEST;
  } else if (headers_in.host != "") {
    return HTTP_BAD_REQUEST;
  } else {
    headers_in.host = value;
    return HTTP_OK;
  }
}

int HttpContentLength(HeadersIn& headers_in, std::string value) {
  char* end_ptr;

  ssize_t content_length_n = std::strtol(value.c_str(), &end_ptr, 10);
  if (headers_in.content_length != "" || end_ptr == value || *end_ptr != 0) {
    return HTTP_BAD_REQUEST;
  } else {
    headers_in.content_length = value;
    headers_in.content_length_n = content_length_n;
    return HTTP_OK;
  }
}

int HttpContentType(HeadersIn& headers_in, const std::string value) {
  if (!IsToken(value, "/; =")) {
    return HTTP_BAD_REQUEST;
  } else {
    headers_in.content_type = value;
    return HTTP_OK;
  }
}

int HttpTransferEncoding(HeadersIn& headers_in, std::string value) {
  if (!IsToken(value, ", ")) {
    return HTTP_BAD_REQUEST;
  } else if (ToCaseInsensitive(Trim(value)) != "chunked") {
    return HTTP_NOT_IMPLEMENTED;
  } else {
    headers_in.transfer_encoding = value;
    headers_in.chuncked = true;
    return HTTP_OK;
  }
}

int HttpConnection(HeadersIn& headers_in, std::string value) {
  if (!IsToken(value, ", ")) {
    return HTTP_BAD_REQUEST;
  } else {
    headers_in.connection = value;
    std::string conntection_list = RemoveWhiteSpace(headers_in.connection);
    std::vector<std::string> connection_options = Split(conntection_list, ',');
    if (std::find(connection_options.begin(), connection_options.end(),
                  HTTP_CONNECTION_OPTION_CLOSE) != connection_options.end()) {
      headers_in.connection_close = true;
    }
    return HTTP_OK;
  }
}
