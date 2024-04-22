#include "utils.hpp"

#include <cstdlib>
#include <sstream>

std::string MakeRfc850Time(const std::time_t& time) {
  std::ostringstream oss;
  std::tm* tm = std::gmtime(&time);  // UTC 시간으로 변환

  if (tm) {
    // Weekday, Day-Month-Year Hour:Minute:Second GMT 형식으로 변환
    const char* weekday_names[] = {"Sun", "Mon", "Tue", "Wed",
                                   "Thu", "Fri", "Sat"};
    const char* month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    oss << weekday_names[tm->tm_wday] << ", ";
    oss << std::setw(2) << std::setfill('0') << tm->tm_mday << "-";
    oss << month_names[tm->tm_mon] << "-";
    oss << std::setw(4) << tm->tm_year + 1900 << " ";
    oss << std::setw(2) << std::setfill('0') << tm->tm_hour << ":";
    oss << std::setw(2) << std::setfill('0') << tm->tm_min << ":";
    oss << std::setw(2) << std::setfill('0') << tm->tm_sec << " GMT";
  }

  return oss.str();
}

std::string ToCaseInsensitive(std::string str) {
  std::string insensitive_str = std::string(str);
  for (size_t i = 0; i < insensitive_str.length(); ++i) {
    insensitive_str[i] = std::tolower(insensitive_str[i]);
  }
  return insensitive_str;
}

std::string RemoveWhiteSpace(std::string str) {
  std::string result;
  for (size_t i = 0; i < str.length(); ++i) {
    char c = str[i];
    if (c != ' ') {
      result += c;
    }
  }
  return result;
}

std::string Trim(const std::string s) {
  size_t front = 0;
  ssize_t back = s.npos;

  for (; front < s.length(); ++front) {
    if (s[front] != 9 && s[front] != 32) {
      break;
    }
  }

  for (; back >= 0; --back) {
    if (s[back] != 9 && s[back] != 32) {
      break;
    }
  }

  return s.substr(front, back);
}

std::vector<std::string> Split(std::string& str, const char delimiter) {
  std::istringstream iss(str);
  std::string buffer;
  std::vector<std::string> buffer_container;

  while (getline(iss, buffer, delimiter)) {
    if (buffer.length() > 0) {
      buffer_container.push_back(buffer);
    }
  }
  return buffer_container;
}

const std::vector<const std::string> Split(const std::string& str,
                                           const char delimiter) {
  std::istringstream iss(str);
  std::string buffer;
  std::vector<const std::string> buffer_container;

  while (getline(iss, buffer, delimiter)) {
    if (buffer.length() > 0) {
      buffer_container.push_back(buffer);
    }
  }
  return buffer_container;
}

std::vector<std::string> Split(std::string& str, const std::string& sep) {
  std::vector<std::string> res;

  size_t idxStart = 0;
  for (size_t idx = 0; idx < str.length(); idx++) {
    if (str.substr(idx, sep.length()).compare(sep) == 0) {
      res.push_back(str.substr(idxStart, idx - idxStart));
      idxStart = idx + sep.length();
      idx += sep.length() - 1;
    }
  }

  if (idxStart != str.length())
    res.push_back(str.substr(idxStart, str.length() - idxStart));

  return res;
}

const std::vector<const std::string> Split(const std::string& str,
                                           const std::string& sep) {
  std::vector<const std::string> res;

  size_t idxStart = 0;
  for (size_t idx = 0; idx < str.length(); idx++) {
    if (str.substr(idx, sep.length()).compare(sep) == 0) {
      res.push_back(str.substr(idxStart, idx - idxStart));
      idxStart = idx + sep.length();
      idx += sep.length() - 1;
    }
  }

  if (idxStart != str.length())
    res.push_back(str.substr(idxStart, str.length() - idxStart));

  return res;
}

void* Memset(void* b, int c, const size_t len) {
  unsigned char* bb;
  unsigned char cc;
  size_t i;

  bb = (unsigned char*)b;
  cc = (unsigned char)c;
  i = 0;

  for (i = 0; i < len; i++) {
    bb[i] = cc;
  }

  return ((void*)b);
}

bool isPositiveInteger(const std::string& str) {
  if (str.empty()) return false;

  for (std::size_t i = 0; i < str.length(); ++i) {
    if (str[i] < '0' || str[i] > '9') return false;
  }

  return std::atoi(str.c_str()) > 0;
}
