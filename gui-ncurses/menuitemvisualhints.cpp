#include "menuitemvisualhints.h"

#include "utils/debugutils.h"
#include "utils/fileutils.h"

namespace TUI {
namespace NCurses {

MenuItemVisualHints::MenuItemVisualHints(const MenuItemPointer item)
    : color(NCursesApplication::ColorDefault)
    , attributes(0)
{
    assert(item);

    if (! item->isEmpty()) {
        const std::string path = item->path();
        Utils::FileUtils::FileInfo fileInfo(path);

        if (fileInfo.exists) {
            if (fileInfo.isDirectory) {
                hint = " Press RETURN to enter the directory ";
                if (NCursesApplication::supportsColors())
                    color = NCursesApplication::ColorBlue;
                else
                    attributes |= A_BOLD;
            } else {
                if (fileInfo.isExecutable && NCursesApplication::supportsColors())
                    color = NCursesApplication::ColorGreen;
                // TODO: Add a proper hint once we can nicely execute command lines
                // in the outer shell function handler.
                hint = " TODO: Proper hint ";
            }
        } else {
            if (NCursesApplication::supportsColors())
                color = NCursesApplication::ColorRed;
            else
                attributes |= A_UNDERLINE;
            hint = " Error: File or directory does not exist ";
        }
    }
}

} // namespace NCurses
} // namespace TUI
