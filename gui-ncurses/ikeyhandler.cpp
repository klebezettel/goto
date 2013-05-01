#include "ikeyhandler.h"

namespace TUI {
namespace NCurses {

bool operator<(const IKeyController::KeyPress &lhs, const IKeyController::KeyPress &rhs)
{
    if (lhs.escapePreceded && !rhs.escapePreceded)
        return true;
    if (!lhs.escapePreceded && rhs.escapePreceded)
        return false;
    return lhs.key < rhs.key;
}

bool operator==(const IKeyController::KeyPress &lhs, const IKeyController::KeyPress &rhs)
{
    return lhs.key == rhs.key && lhs.escapePreceded == rhs.escapePreceded;
}

} // namespace NCurses
} // namespace TUI
