#include "gotoapplication.h"

using namespace TUI::NCurses;

static bool isQuitKeyPress(IKeyController::KeyPress keyPress)
{
    return keyPress == IKeyController::KeyPress('`', true)
        || keyPress == IKeyController::KeyPress(KEY_CTRL_C)
        || keyPress == IKeyController::KeyPress(KEY_CTRL_D);
}

bool GotoApplication::handleKey(IKeyController::KeyPress keyPress)
{
    if (isQuitKeyPress(keyPress)) {
        exit();
        return true;
    }
    return false;
}
