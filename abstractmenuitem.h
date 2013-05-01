#ifndef ABSTRACTMENUITEM_H
#define ABSTRACTMENUITEM_H

#include <memory>
#include <string>
#include <vector>

class AbstractMenuItem
{
public:
    virtual ~AbstractMenuItem() {}

    virtual std::string identifier() const = 0;
    virtual std::string path() const = 0;
    virtual std::string pathDisplayed() const { return path(); }

    virtual bool isEmpty() { return identifier().empty() && path().empty(); }
};

typedef std::shared_ptr<AbstractMenuItem> MenuItemPointer;
typedef std::vector<MenuItemPointer> MenuItems;

#endif // ABSTRACTMENUITEM_H
