#ifndef BOOKMARKMENU_H
#define BOOKMARKMENU_H

#include "filtermenu.h"
#include "ncursesapplication.h"

#include <memory>
#include <string>

namespace TUI {
namespace NCurses {

class BookmarkItem : public AbstractMenuItem {
public:
    class PathHandlerHint
    {
    public:
        PathHandlerHint(const std::string &path);

        enum Hint {
            NoHandlerHint,
            ChangeToDirectory,
            ExecuteApplication,
            OpenWithDefaultApplication
        } hint;
    };

    BookmarkItem(const std::string name, const std::string path) : m_name(name), m_path(path) {}
    std::string identifier() const { return m_name; }
    std::string path() const { return m_path; }
    std::string pathDisplayed() const;

private:
    const std::string m_name;
    const std::string m_path;
};

typedef std::shared_ptr<BookmarkItem> BookmarkItemPointer;

class BookmarkMenu : public FilterMenu
{
public:
    BookmarkMenu(const std::string &bookmarkFilePath, const MenuItems menuItems = MenuItems(),
                 IKeyHandler *parentKeyHandler = 0);
    bool openEditor();

    BookmarkItemPointer chosenItem();
private:
    void readBookmarksFromFile();

    const std::string m_bookmarkFilePath;
};

} // namespace NCurses
} // namespace TUI

#endif // BOOKMARKMENU_H
