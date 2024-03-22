#include "../src/url.hpp"

int TestParseScheme(Url& url, std::string& scheme) {
  return url.ParseScheme(scheme);
}
int TestParseAuthority(Url& url, std::string& authority) {
  return url.ParseAuthority(authority);
}

int main() {
  Url url;

  std::cout << "START THE TEST\n\n";

  std::string scheme = "http";
  if (TestParseScheme(url, scheme)) {
    std::cout << "test ParseScheme() failed by <" << scheme << ">\n";
  }

  std::string authority = "127.0.0.1";
  if (TestParseAuthority(url, authority)) {
    std::cout << "test ParseAuthority() failed by <" << authority << ">\n";
  }

  std::cout << "\nEND THE TEST\n";
  return 0;
}
