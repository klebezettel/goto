#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <algorithm> // find_if
#include <stdexcept>
#include <string>

// For stat():
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Utils {
namespace FileUtils {

void writeFile(const std::string filePath, const std::string fileContents) throw(std::runtime_error);

// TODO: Make this portable.
class FileInfo
{
public:
    FileInfo(const std::string &filePath);

    bool exists : 1;
    bool isRegularFile : 1;
    bool isDirectory : 1;
    bool isExecutable : 1;
};

} // namespace FileUtils
} // namespace Utils

#endif // FILEUTILS_H
