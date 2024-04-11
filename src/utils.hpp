#ifndef UTILS_HPP
#define UTILS_HPP

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

std::string MakeRfc850Time(const std::time_t& time);
std::string ToCaseInsensitive(std::string str);

std::string Trim(std::string s);

std::vector<std::string> Split(std::string& str, const char delimiter);
const std::vector<const std::string> Split(const std::string& str,
                                           const char delimiter);
std::vector<std::string> Split(std::string& str, const std::string& sep);
const std::vector<const std::string> Split(const std::string& str,
                                           const std::string& sep);

void* Memset(void* b, const int c, const size_t len);

#endif
