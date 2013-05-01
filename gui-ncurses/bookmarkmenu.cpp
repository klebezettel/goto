#include "bookmarkmenu.h"

#include "utils/debugutils.h"
#include "utils/fileutils.h"
#include "utils/stringutils.h"

#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace TUI {
namespace NCurses {

BookmarkMenu::BookmarkMenu(const std::string &bookmarkFilePath, const MenuItems menuItems,
                           IKeyHandler *parentKeyHandler)
    : FilterMenu(menuItems, parentKeyHandler)
    , m_bookmarkFilePath(bookmarkFilePath)
{
    readBookmarksFromFile();
    m_map[IKeyHandler::KeyPress('e', true)] = std::bind(&BookmarkMenu::openEditor, this);
}

bool BookmarkMenu::openEditor()
{
    std::stringstream command;
    command << "$EDITOR " << "$HOME/" << m_bookmarkFilePath;

    NCursesApplication::runExternalCommand(command.str());
    readBookmarksFromFile();
    reset(); // Cursor might be on the last entry and the user might deleted the last entry.

    return true;
}

BookmarkItemPointer BookmarkMenu::chosenItem()
{
    return std::static_pointer_cast<BookmarkItem>(FilterMenu::chosenItem());
}

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

void BookmarkMenu::readBookmarksFromFile()
{
    const std::string homePath = std::getenv("HOME");
    const std::string filePath = homePath + std::string("/") + m_bookmarkFilePath;

    // Read file
    std::ifstream file(filePath.c_str());
    if (! file)
        throw std::runtime_error("Could not open file \"" + filePath + "\"");

    // Parse file
    m_allMenuItems.clear();
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
                NCursesApplication::error(reason);
            }
        }

        // Don't trim in the beginning. User might want to indent.
        Utils::StringUtils::rtrim(bookmarkName);
        Utils::StringUtils::trim(bookmarkPath);

        m_allMenuItems.push_back(BookmarkItemPointer(new BookmarkItem(bookmarkName, bookmarkPath)));
    }

    // Discard only line or last line if it is empty.
    if (! m_allMenuItems.empty() && m_allMenuItems.back()->isEmpty())
            m_allMenuItems.pop_back();

    m_menuItems = m_allMenuItems;
    file.close();
}

} // namespace NCurses
} // namespace TUI
