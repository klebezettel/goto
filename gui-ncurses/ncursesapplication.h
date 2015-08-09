#ifndef NCURSESAPPLICATION_H
#define NCURSESAPPLICATION_H

#include "ncurses.h"

#include <cstdlib>
#include <string>

namespace TUI {
namespace NCurses {

const int KEY_CTRL_C = 3;
const int KEY_CTRL_D = 4;
const int KEY_ESC = 27;
const int KEY_RETURN = 10;

// TODO: Extract some methods to "NCursesUtils"
class NCursesApplication
{
public:
    NCursesApplication();
    ~NCursesApplication();

    enum Color {
        ColorDefault,
        ColorRed,
        ColorGreen,
        ColorYellow,
        ColorBlue,
        ColorMagenta,
        ColorCyan,
        ColorWhite
    };

    static bool supportsColors();
    static void useColor(WINDOW *window, Color color);

    static void runExternalCommand(const std::string &command);

    static void error(const std::string errorMessage);
    static void exit(int exitCode = EXIT_SUCCESS);

    static void maybeChop(const WINDOW *window, int startPosition, std::string &text);
};

} // namespace NCurses
} // namespace TUI

#endif // NCURSESAPPLICATION_H
