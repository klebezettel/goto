#ifndef BOOKMARKITEMSMODEL_H
#define BOOKMARKITEMSMODEL_H

#include "imenuitem.h"
#include "imodel.h"

#include <string>

namespace Core {

class BookmarkItem : public IMenuItem {
public:
    class HandlerHint
    {
    public:
        HandlerHint(const std::string &path);

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

class BookmarkItemsModel: public IModel
{
public:
    BookmarkItemsModel(const std::string &bookmarkFilePath, bool refresh = true);

    MenuItems items(bool refresh = false);

private:
    void readBookmarksFromFile();

    std::string m_bookmarkFilePath;
    MenuItems m_items;
};

} // namespace Core

#endif // BOOKMARKITEMSMODEL_H
