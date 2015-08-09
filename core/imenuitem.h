#ifndef IMENUITEM_H
#define IMENUITEM_H

#include <memory>
#include <string>
#include <vector>

class IMenuItem
{
public:
    virtual ~IMenuItem() {}

    virtual std::string identifier() const = 0;
    virtual std::string path() const = 0;
    virtual std::string pathDisplayed() const { return path(); }

    virtual bool isEmpty() { return identifier().empty() && path().empty(); }
};

using MenuItemPointer = std::shared_ptr<IMenuItem>;
using MenuItems = std::vector<MenuItemPointer>;

#endif // IMENUITEM_H
