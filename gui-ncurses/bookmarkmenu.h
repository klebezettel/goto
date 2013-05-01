#ifndef BOOKMARKMENU_H
#define BOOKMARKMENU_H

#include "filtermenu.h"
#include "ncursesapplication.h"

#include "core/bookmarkitemsmodel.h"

#include <memory>
#include <string>

namespace TUI {
namespace NCurses {

class BookmarkMenu : public FilterMenu
{
public:
    BookmarkMenu(const std::string &bookmarkFilePath, Core::BookmarkItemsModel &model,
                 IKeyController *parentKeyHandler = 0);
    bool openEditor();

    Core::BookmarkItemPointer chosenItem();

private:
    const std::string m_bookmarkFilePath;
};

} // namespace NCurses
} // namespace TUI

#endif // BOOKMARKMENU_H
