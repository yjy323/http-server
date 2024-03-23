#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <string>
#include <vector>

void ToCaseInsensitve(std::string& str);

const std::vector<const std::string> Split(const std::string& str,
                                           const char delimiter);
const std::vector<const std::string> Split(const std::string& str,
                                           const std::string& sep);

#endif
