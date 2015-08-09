#ifndef GOTOAPPLICATION_H
#define GOTOAPPLICATION_H

#include <gui-ncurses/ikeyhandler.h>
#include <gui-ncurses/ncursesapplication.h>

class GotoApplication
    : public TUI::NCurses::NCursesApplication
    , public TUI::NCurses::IKeyController
{
public:
    bool handleKey(KeyPress keyPress);
};

#endif // GOTOAPPLICATION_H
