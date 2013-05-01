#ifndef IKEYHANDLER_H
#define IKEYHANDLER_H

namespace TUI {
namespace NCurses {

class IKeyController
{
public:
    struct KeyPress {
        KeyPress(int key, bool escapePreceded = false)
            : key(key), escapePreceded(escapePreceded) {}

        int key;
        const bool escapePreceded;
    };

    virtual bool handleKey(KeyPress keyPress) = 0;
};

bool operator<(const IKeyController::KeyPress &lhs, const IKeyController::KeyPress &rhs);
bool operator==(const IKeyController::KeyPress &lhs, const IKeyController::KeyPress &rhs);

} // namespace NCurses
} // namespace TUI

#endif // IKEYHANDLER_H
