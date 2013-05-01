/// Requirements
///
/// Listing Bookmarks mode:
///  <number> <name> <fullpath>
///
/// Config file is ~/.goto.bookmarks
///
/// Keys:
///   Up, k, Down, j:    Move up and down the list.
///   Home, End:         Select first, last item.
///   Enter:             Go to selected directory.
///   Digit:             Select item with by digit.
///   e:                 Open editor with bookmarks file.
///   TODO: i:           Enter filter mode. You can enter a pattern
///                      and the filtered list will be shown.
///                      In filter Mode:
///                          Press any printable character to add it to the filter.
///                          ESC aborts filtering.
///   Esc:               Exit, e.g. stay in $CWD (or abort filter in filter mode).
///
///  Fundamental use cases:
///   (1) Navigate to a directory
///   (2) Open/Edit a file
///   (3) Give string of selected files/dirs. This is useful for
///        Launching programs which are not in PATHs
///        Parameterizing programs with files/paths
///
///  Mode bar for use case 1/2: Bookmarks - Browse - Find Files - Locate Files
///  Mode bar for use case 3: Path Chooser
///
///  General:
///  TODO: Entering [a-zA-Z] starts filter (no j,k commands, but modifier needed!)
///  TODO: First start screen (for instructions)
///  TODO: Help screen with all short cuts and modes.
///  TODO: -help
///  TODO: man page
///  TODO: Home page
///  TODO: Settings file
///     Option: remember last selected entry
///     Option: Enable/Disable/Show digit accesor column (first column)
///
///  Platforms:
///  TODO: Support for Mac OS X
///  TODO: Support for Cygwin on Windows?
///  TODO: Support for Windows?
///
///  Building/Packaging:
///  TODO: Packages for: Debian, Ubuntu, SuSE, Fedora, Arch Linux, Gentoo
///  TODO: Get rid of qmake dependency
///
///  Bookmarks:
///  TODO: Comments in bookmark file (initial file contains format description in comment and
///        some examples)
///  TODO: Enhance to general launcher/executer
///        File: (1) Open with $EDITOR (2) Open with xdg-open (3) Execute if execute bit is set!
///         Dir: (1) Go to location (2) Open with xdg-open (for image dirs e.g.)
///  TODO: Directory dependent bookmarks/actions!
///
///  Display:
///  TODO: Digit navigation for all entries? (line numbers)
///
///  TEST: Call with empty file (--> welcome screen)
///  TEST: Call with no file (--> welcome screen)
///  TEST: Call with malformed line
///
///  TODO: When crashing, try to shutdown ncurses properly. Otherwise artifacts will be produced.
///
///  IDEA: multiple 'book mark files' - select on start which to use or at run time which to use
///      --> showing as tabs?

#include "core/bookmarkitemsmodel.h"

#include "gui-ncurses/bookmarkmenu.h"
#include "gui-ncurses/ikeyhandler.h"
#include "gui-ncurses/ncursesapplication.h"

#include "utils/debugutils.h"
#include "utils/fileutils.h"
#include "utils/stringutils.h"

using namespace std;
using namespace TUI::NCurses;

static const char BookmarkFile[] = ".goto.bookmarks";
static const char ResultFile[] = ".goto.result";

class GotoApplication : public NCursesApplication, public IKeyController
{
public:
    bool handleKey(KeyPress keyPress)
    {
        if (keyPress == KeyPress('`', true) || keyPress == KeyPress(KEY_CTRL_C)
                || keyPress == KeyPress(KEY_CTRL_D)) {
            exit();
            return true;
        }
        return false;
    }
};

int main(int argc, char *argv[])
{
    GotoApplication app;

    Core::BookmarkItemsModel bookmarkItemsModel(BookmarkFile);
    BookmarkMenu menu(BookmarkFile, bookmarkItemsModel, &app);
    menu.exec(); // Block until the user decided for an item.

    Core::BookmarkItemPointer item = menu.chosenItem();
    assert(item);
    Core::BookmarkItem::HandlerHint handlerHint(item->path());
    assert(handlerHint.hint != Core::BookmarkItem::HandlerHint::NoHandlerHint);

    // Check format for resulting file
    enum ResultFileFormat { WriteInDefaultFormat, WriteInFutureFormat } resultFileFormat;
    resultFileFormat = WriteInDefaultFormat;
    if (argc == 2 && string("--future-format") == argv[1])
        resultFileFormat = WriteInFutureFormat;

    string fileContents;
    if (resultFileFormat == WriteInDefaultFormat) {
        fileContents = item->path();
    } else {
        // TODO: Make this portable
        switch (handlerHint.hint) {
        case Core::BookmarkItem::HandlerHint::ChangeToDirectory:
            fileContents = "cd \"" + item->path() + '"';
            break;
        case Core::BookmarkItem::HandlerHint::ExecuteApplication:
            fileContents = '"' + item->path() + '"';
            break;
        case Core::BookmarkItem::HandlerHint::OpenWithDefaultApplication:
            fileContents = "xdg-open \"" + item->path() + '"';
            break;
        default:
            fileContents = "Ops, could not determine command to handle path \""
                + item->path() + "\".";
        }
    }

    // No result file is written if the user aborts by e.g. Ctrl-C since we
    // never will get to this point. This is OK since the shell function
    // handles this case.
    const string filePath = string(getenv("HOME")) + "/" + ResultFile;
    Utils::FileUtils::writeFile(filePath, fileContents);

    return EXIT_SUCCESS;
}
