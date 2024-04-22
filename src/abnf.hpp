#ifndef ABNF_HPP
#define ABNF_HPP

#include <string>

bool IsOctet(char c);
bool IsHost(std::string s);
bool IsWhiteSpace(char c);
bool IsVchar(char c);
bool IsObsText(unsigned char c);
bool IsToken(std::string s, std::string delimiter);
bool IsUnreserved(char c);
bool IsSubDlims(char c);
bool IsPctEncoded(std::string s, size_t pos);

#endif
