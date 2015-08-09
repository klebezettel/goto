#include "fileutils.h"

#include <fstream>

namespace Utils {
namespace FileUtils {

void writeFile(const std::string filePath, const std::string fileContents) throw(std::runtime_error)
{
    std::ofstream file(filePath);
    if (! file)
        throw std::runtime_error("Could not open file \"" + filePath + "\" for writing");
    file << fileContents << std::flush;
    if (! file)
        throw std::runtime_error("Failed to write file  \"" + filePath + "\"");
}

FileInfo::FileInfo(const std::string &filePath)
    : exists(false), isRegularFile(false), isDirectory(false), isExecutable(false)
{
    struct stat s;
    const int err = stat(filePath.c_str(), &s);
    if (err == -1) {
        if (ENOENT != errno) {
            perror("stat");
            exit(1);
        }
    } else {
        exists = true;
        if (s.st_mode & S_IXUSR) // TODO: This is not enough if we are not the owner.
            isExecutable = true;
        if (s.st_mode & S_IFDIR )
            isDirectory = true;
        else if (s.st_mode & S_IFREG )
            isRegularFile = true;
    }
}

} // namespace FileUtils
} // namespace Utils



