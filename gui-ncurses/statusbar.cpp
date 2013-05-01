#include "statusbar.h"

namespace TUI {
namespace NCurses {

StatusBar::StatusBar(int rows, int columns, int beginY, int beginX)
    : m_textAttributes(0)
    , m_window(newwin(rows, columns, beginY, beginX))
{
}

StatusBar::~StatusBar()
{
    delwin(m_window);
}

void StatusBar::setText(const std::string &text, int attributes, NCursesApplication::Color color)
{
    m_text = text;
    m_textAttributes = attributes;
    m_textColor = color;
}

void StatusBar::update()
{
    wattrset(m_window, 0);
    wattron(m_window, A_BOLD);
    mvwhline(m_window, 0, 0, ACS_HLINE, 1000); // TODO: Is it OK to use NCURSES_ACS?

    wattrset(m_window, 0);
    NCursesApplication::useColor(m_window, m_textColor);
    wattron(m_window, m_textAttributes);
    mvwprintw(m_window, 0, 3, m_text.c_str());

    wattrset(m_window, 0);
    wrefresh(m_window);
}

} // namespace NCurses
} // namespace TUI
