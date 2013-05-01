#ifndef MENUITEMVISUALHINTS_H
#define MENUITEMVISUALHINTS_H

#include "ncursesapplication.h"

#include "core/imenuitem.h"

#include <string>

namespace TUI {
namespace NCurses {

class MenuItemVisualHints
{
public:
    MenuItemVisualHints(const MenuItemPointer item);

    NCursesApplication::Color color;
    int attributes;
    std::string hint;
};

} // namespace NCurses
} // namespace TUI

#endif // MENUITEMVISUALHINTS_H
