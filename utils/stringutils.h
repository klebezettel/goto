#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>

namespace Utils {
namespace StringUtils {

std::string &ltrim(std::string &s);
std::string &rtrim(std::string &s);
std::string &trim(std::string &s);

} // namespace StringUtils
} // namespace Utils

#endif // STRINGUTILS_H
