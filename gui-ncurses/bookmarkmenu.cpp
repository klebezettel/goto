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

BookmarkMenu::BookmarkMenu(const std::string &bookmarkFilePath,
                           Core::BookmarkItemsModel &model,
                           IKeyController *parentKeyHandler)
    : FilterMenu(model, parentKeyHandler)
    , m_bookmarkFilePath(bookmarkFilePath)
{
    m_map[IKeyController::KeyPress('e', true)] = std::bind(&BookmarkMenu::openEditor, this);
}

bool BookmarkMenu::openEditor()
{
    std::stringstream command;
    command << "$EDITOR " << "$HOME/" << m_bookmarkFilePath;

    NCursesApplication::runExternalCommand(command.str());
    m_allMenuItems = m_menuItems = m_model.items(true); // Reread file contents.
    reset(); // Cursor might be on the last entry and the user might deleted the last entry.

    return true;
}

Core::BookmarkItemPointer BookmarkMenu::chosenItem()
{
    return std::static_pointer_cast<Core::BookmarkItem>(FilterMenu::chosenItem());
}

} // namespace NCurses
} // namespace TUI
