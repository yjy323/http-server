#ifndef HTTP_HPP
#define HTTP_HPP

#include <cstdlib>
#include <ctime>
#include <list>
#include <map>
#include <string>

#define CRLF "\r\n"

#define HTTP_GET_METHOD "GET\0"
#define HTTP_POST_METHOD "POST\0"
#define HTTP_DELETE_METHOD "DELETE\0"

#define HTTP_1_1 "HTTP/1.1\0"
#define HTTP_1_0 "HTTP/1.0\0"

#define HTTP_INFORMATIONAL 1
#define HTTP_SUCCESSFUL 2
#define HTTP_REDIRECTION 3
#define HTTP_CLIENT_ERROR 4
#define HTTP_SERVER_ERROR 5

#define HTTP_CONTINUE 100
#define HTTP_SWITCHING_PROTOCOLS 101
#define HTTP_PROCESSING 102

#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_ACCEPTED 202
#define HTTP_NO_CONTENT 204
#define HTTP_PARTIAL_CONTENT 206

#define HTTP_SPECIAL_RESPONSE 300
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_MOVED_TEMPORARILY 302
#define HTTP_SEE_OTHER 303
#define HTTP_NOT_MODIFIED 304
#define HTTP_TEMPORARY_REDIRECT 307
#define HTTP_PERMANENT_REDIRECT 308

#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_NOT_ALLOWED 405
#define HTTP_REQUEST_TIME_OUT 408
#define HTTP_CONFLICT 409
#define HTTP_LENGTH_REQUIRED 411
#define HTTP_PRECONDITION_FAILED 412
#define HTTP_REQUEST_ENTITY_TOO_LARGE 413
#define HTTP_REQUEST_URI_TOO_LARGE 414
#define HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define HTTP_RANGE_NOT_SATISFIABLE 416
#define HTTP_MISDIRECTED_REQUEST 421
#define HTTP_TOO_MANY_REQUESTS 429

#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_NOT_IMPLEMENTED 501
#define HTTP_BAD_GATEWAY 502
#define HTTP_SERVICE_UNAVAILABLE 503
#define HTTP_GATEWAY_TIME_OUT 504
#define HTTP_VERSION_NOT_SUPPORTED 505
#define HTTP_INSUFFICIENT_STORAGE 507

#define HTTP_CONNECTION_OPTION_CLOSE "close\0"
#define HTTP_CONNECTION_OPTION_KEEP_ALIVE "keep-alive\0"

const static char* REASON_PHASE[] = {"Continue",
                                     "Switching Protocols",
                                     "Processing",
                                     "OK",
                                     "Created",
                                     "Accepted",
                                     "No Content",
                                     "Partial Content",
                                     "Special Response",
                                     "Moved Permanently",
                                     "Moved Temporarily",
                                     "See Other",
                                     "Not Modified",
                                     "Temporary Redirect",
                                     "Permanent Redirect",
                                     "Bad Request",
                                     "Unauthorized",
                                     "Forbidden",
                                     "Not Found",
                                     "Method Not Allowed",
                                     "Request Time-out",
                                     "Conflict",
                                     "Length Required",
                                     "Precondition Failed",
                                     "Request Entity Too Large",
                                     "Request-URI Too Large",
                                     "Unsupported Media Type",
                                     "Range Not Satisfiable",
                                     "Misdirected Request",
                                     "Too Many Requests",
                                     "Internal Server Error",
                                     "Not Implemented",
                                     "Bad Gateway",
                                     "Service Unavailable",
                                     "Gateway Time-out",
                                     "HTTP Version Not Supported",
                                     "Insufficient Storage"};

typedef const std::string HeaderKey;
typedef std::list<const std::string> HeaderValue;
typedef std::map<HeaderKey, HeaderValue>::iterator HeadersIterator;
typedef std::map<HeaderKey, HeaderValue>::const_iterator HeadersConstIterator;

struct HeadersIn {
  std::map<HeaderKey, HeaderValue> headers_;

  std::string host;
  std::string connection;
  std::string if_modified_since;
  std::string if_unmodified_since;
  std::string if_match;
  std::string if_none_match;
  std::string user_agent;
  std::string referer;
  std::string content_length;
  std::string content_range;
  std::string content_type;

  std::string range;
  std::string if_range;

  std::string transfer_encoding;
  std::string te;
  std::string expect;
  std::string upgrade;

  ssize_t content_length_n;
  bool connection_close;
  bool chuncked;
};

struct HeadersOut {
  std::string server;
  std::string date;
  std::string content_length;
  std::string content_encoding;
  std::string content_type;
  std::string location;
  std::string refresh;
  std::string last_modified;
  std::string content_range;
  std::string accept_ranges;
  std::string www_authenticate;
  std::string expires;
  std::string etag;
  std::string allow;
  std::string connection;

  ssize_t content_length_n;
  bool connection_close;
  std::time_t date_t;
};

const char* HttpGetReasonPhase(int status_code);

const std::string HttpInsertHeader(std::map<HeaderKey, HeaderValue>& headers,
                                   const std::string key,
                                   const std::string value);

int HttpHost(HeadersIn&, const std::string);
int HttpContentType(HeadersIn&, const std::string);
int HttpContentLength(HeadersIn&, const std::string);
int HttpTransferEncoding(HeadersIn&, const std::string);
int HttpConnection(HeadersIn&, const std::string);

#endif
