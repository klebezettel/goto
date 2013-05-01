#include "ncursesapplication.h"

#include <iostream>

namespace TUI {
namespace NCurses {

NCursesApplication::NCursesApplication()
{
    initscr();
    start_color();
    raw(); // Pass through all keys (interrupt, quit, suspend and flow control)
    noecho();
    keypad(stdscr, TRUE); // Enables us to catch arrow presses.
    curs_set(0);

    if (has_colors()) {
        // Enable '-1' in the following lines for the default terminal color.
        use_default_colors();

        init_pair(ColorDefault, COLOR_BLACK, -1);
        init_pair(ColorRed, COLOR_RED, -1);
        init_pair(ColorGreen, COLOR_GREEN, -1);
        init_pair(ColorYellow, COLOR_YELLOW, -1);
        init_pair(ColorBlue, COLOR_BLUE, -1);
        init_pair(ColorMagenta, COLOR_MAGENTA, -1);
        init_pair(ColorCyan, COLOR_CYAN, -1);
        init_pair(ColorWhite, COLOR_WHITE, -1);
    }
}

NCursesApplication::~NCursesApplication()
{
    shutdownNCurses();
}

bool NCursesApplication::supportsColors()
{
    // The function can_change_color() returns false for
    //   gnome-terminal 3.6.0
    //   XTerm(278)
    //   konsole 2.9.4
    // Therefore we will have no colors on these terminal
    // if we rely on that.
    return has_colors() /*&& can_change_color()*/;
}

void NCursesApplication::useColor(WINDOW *window, Color color)
{
    if (supportsColors())
         wcolor_set(window, color, 0);
}

void NCursesApplication::runExternalCommand(const std::string &command)
{
    shutdownNCurses();
    system(command.c_str());
    resumeNCurses();
}

void NCursesApplication::error(const std::string errorMessage)
{
    shutdownNCurses();
    std::cerr << "Error: " << errorMessage << '.' << std::endl;
    ::exit(EXIT_FAILURE);
}

void NCursesApplication::exit(int exitCode)
{
    shutdownNCurses();
    ::exit(exitCode);
}

void NCursesApplication::maybeChop(const WINDOW *window, int startPosition, std::string &text)
{
    int windowColumns, windowRows;
    getmaxyx(window, windowRows, windowColumns);
    (void) windowRows; // Use the unused.

    if (startPosition < windowColumns) {
        const unsigned charsToLeave = windowColumns - startPosition;
        if (text.size() > charsToLeave)
            text.erase(text.begin() + charsToLeave, text.end());
    } else {
        text.clear();
    }
}

void NCursesApplication::shutdownNCurses()
{
    endwin();
}

void NCursesApplication::resumeNCurses()
{
    refresh();
}

} // namespace NCurses
} // namespace TUI
