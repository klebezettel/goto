#include "bookmarkitemsmodel.h"

#include "utils/debugutils.h"
#include "utils/fileutils.h"
#include "utils/stringutils.h"

#include <sstream>
#include <stdexcept>

namespace Core {

BookmarkItem::PathHandlerHint::PathHandlerHint(const std::string &path)
    : hint(NoHandlerHint)
{
    assert(!path.empty());
    Utils::FileUtils::FileInfo fileInfo(path);
    assert(fileInfo.exists);

    if (fileInfo.isDirectory) {
        hint = ChangeToDirectory;
    } else {
        hint = fileInfo.isExecutable
            ? ExecuteApplication : OpenWithDefaultApplication;
    }
}

std::string BookmarkItem::pathDisplayed() const
{
    const std::string homePath = std::getenv("HOME");
    const int homePathLength = homePath.size();
    const std::string path = this->path();
    if (! path.compare(0, homePathLength, homePath))
        return std::string(path).replace(0, homePathLength, "~");
    return path;
}

BookmarkItemsModel::BookmarkItemsModel(const std::string &bookmarkFilePath, bool refresh)
    : m_bookmarkFilePath(bookmarkFilePath)
{
    if (refresh)
        readBookmarksFromFile();
}

MenuItems BookmarkItemsModel::items(bool refresh)
{
    if (refresh)
        readBookmarksFromFile();
    return m_items;
}

void BookmarkItemsModel::readBookmarksFromFile()
{
    const std::string homePath = std::getenv("HOME");
    const std::string filePath = homePath + std::string("/") + m_bookmarkFilePath;

    // Read file
    std::ifstream file(filePath.c_str());
    if (! file)
        throw std::runtime_error("Could not open file \"" + filePath + "\"");

    // Parse file
    m_items.clear();
    std::string line;
    const char delimiter = ',';
    bool lastLineWasEmpty = false;
    for (unsigned lineNumber = 1; file && getline(file, line); ++lineNumber) {
        // Merge multiple empty lines to one entry
        std::string copiedLine(line);
        const bool isEmptyLine = Utils::StringUtils::trim(copiedLine).empty();
        if (lastLineWasEmpty) {
            if (isEmptyLine)
                continue;
            else
                lastLineWasEmpty = false;
        } else {
            if (isEmptyLine)
                lastLineWasEmpty = true;
        }

        // Parse line
        std::string bookmarkName;
        std::string bookmarkPath;
        if (! isEmptyLine) {
            std::istringstream lineStream(line);
            const bool gotBookmarkName = getline(lineStream, bookmarkName, delimiter);
            const bool gotBookmarkPath = getline(lineStream, bookmarkPath, delimiter);
            if (! gotBookmarkName || ! gotBookmarkPath) {
                const std::string reason = "Malformed line " + std::to_string(lineNumber) + " in "
                        + filePath;
                throw std::runtime_error(reason);
            }
        }

        // Don't trim in the beginning. User might want to indent.
        Utils::StringUtils::rtrim(bookmarkName);
        Utils::StringUtils::trim(bookmarkPath);

        m_items.push_back(BookmarkItemPointer(new BookmarkItem(bookmarkName, bookmarkPath)));
    }

    // Discard only line or last line if it is empty.
    if (! m_items.empty() && m_items.back()->isEmpty())
            m_items.pop_back();

    file.close();
}

} // namespace Core
