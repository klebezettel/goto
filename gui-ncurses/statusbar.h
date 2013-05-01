#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "ncursesapplication.h"

#include <string>

#include <ncurses.h>

namespace TUI {
namespace NCurses {

class StatusBar
{
public:
    StatusBar(int rows, int columns, int beginY, int beginX);
    ~StatusBar();

    void setText(const std::string &text, int attributes, NCursesApplication::Color color);
    void update();

private:
    std::string m_text;
    int m_textAttributes;
    NCursesApplication::Color m_textColor;
    WINDOW *m_window;
};

} // namespace NCurses
} // namespace TUI

#endif // STATUSBAR_H
