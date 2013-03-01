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
///  DONE: Show path abbreviated (~ instead of $HOME).
///  TODO: Digit navigation for all entries? (line numbers)
///  TODO: Highlight not existent dirs (red).
///  TODO: Highlight bookmark if it's $CWD.
///
///  TEST: Call with empty file (--> welcome screen)
///  TEST: Call with no file (--> welcome screen)
///  TEST: Call with malformed line
///
///  IDEA: multiple 'book mark files' - select on start which to use or at run time which to use
///      --> showing as tabs?

#include <cctype>
#include <cstdlib>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include <ncurses.h>

// For stat():
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

static const int KEY_ESC = 27;
static const int KEY_RETURN = 10;
static const char BookmarkFile[] = ".goto.bookmarks";
static const char ResultFile[] = ".goto.result";

// --- Debug & Assert -----------------------------------------------------------------------------

#define DEBUG_OUTPUT 1

/// Ncurses apps cannot just print to stdout/stderr, so print to a file
class DebugOutput
{
#if DEBUG_OUTPUT
    ofstream *stream;
#endif
public:
#if DEBUG_OUTPUT
    explicit DebugOutput(ofstream *stream = 0) : stream(stream) {}
    ~DebugOutput() { *stream << endl << flush; }
#else
    explicit DebugOutput(ofstream * = 0) {}
#endif

#if DEBUG_OUTPUT
#define OUT_OPERATOR(type) \
    DebugOutput &operator<<(const type value) \
    { \
        *stream << value << ' '; \
        return *this; \
    }
#else
#define OUT_OPERATOR(type) \
    DebugOutput &operator<<(const type) { return *this; }
#endif

    OUT_OPERATOR(string&)
    OUT_OPERATOR(wostream&) // for endl
    OUT_OPERATOR(char*)
    OUT_OPERATOR(bool)
    OUT_OPERATOR(int)
};

/// debug(): Return DebugOutput object for printing. Usage: debug() << "hello";
static DebugOutput debug() {
#if DEBUG_OUTPUT
    static ofstream stream("/tmp/goto_debug.log");
    return DebugOutput(&stream);
#else
    return DebugOutput();
#endif
}

/// assert(condition): Ensure condition is true, otherwise print the failed condition with debug().
#define assert(condition) assert_helper(condition, __FILE__, __LINE__)
#define assert_helper(condition, file, line) \
    if (!(condition)) { debug() << file << line << "Assertion failed: " << #condition; }

// --- Utils --------------------------------------------------------------------------------------

namespace Utils {

// TODO: Not portable
class FileInfo
{
public:
    FileInfo(const string &filePath)
        : exists(false), isRegularFile(false), isDirectory(false), isExecutable(false)
    {
        struct stat s;
        const int err = stat(filePath.c_str(), &s);
        if(err == -1) {
            if(ENOENT != errno) {
                perror("stat");
                exit(1);
            }
        } else {
            exists = true;
            if (s.st_mode & S_IXUSR) // TODO: This is not enough if we are not the owner.
                isExecutable = true;
            if (s.st_mode & S_IFDIR )
                isDirectory = true;
            else if (s.st_mode & S_IFREG )
                isRegularFile = true;
        }
    }

    bool exists : 1;
    bool isRegularFile : 1;
    bool isDirectory : 1;
    bool isExecutable : 1;
};

static inline string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(
        s.begin(),
        s.end(),
        std::not1(std::ptr_fun<int, int>(std::isspace)))
    );
    return s;
}

static inline string &rtrim(std::string &s) {
    s.erase(std::find_if(
                s.rbegin(),
                s.rend(),
                std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
            s.end());
    return s;
}

static inline string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

} // namespace Utils

// --- Application classes ------------------------------------------------------------------------

class NCursesApplication
{
public:
    enum Color {
        ColorDefault, ColorRed, ColorGreen, ColorYellow,
        ColorBlue, ColorMagenta, ColorCyan, ColorWhite
    };

    NCursesApplication()
    {
        initscr();
        start_color();
        cbreak();
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

    ~NCursesApplication() { shutdownNCurses(); }

    static bool supportsColors()
    {
        // The function can_change_color() returns false for
        //   gnome-terminal 3.6.0
        //   XTerm(278)
        //   konsole 2.9.4
        // Therefore we will have no colors on these terminal
        // if we rely on that.
        return has_colors() /*&& can_change_color()*/;
    }

    static void useColor(WINDOW *window, Color color)
    {
        if (supportsColors())
             wcolor_set(window, color, 0);
    }

    static void runExternalCommand(const string &command)
    {
        shutdownNCurses();
        system(command.c_str());
        resumeNCurses();
    }

    static void error(const string errorMessage)
    {
        shutdownNCurses();
        cerr << "Error: " << errorMessage << '.' << endl;
        ::exit(EXIT_FAILURE);
    }

    static void exit(int exitCode = EXIT_SUCCESS)
    {
        shutdownNCurses();
        ::exit(exitCode);
    }

private:
    // Prefer to use destructor, not this one.
    static void shutdownNCurses() { endwin(); }
    static void resumeNCurses() { refresh(); }
};

static inline string getEnvironmentVariableOrDie(const char * const name)
{
    const char * const value = getenv(name);
    if (value == NULL)
        NCursesApplication::error("Could not read environment variable " + string(name));
    return value;
}

class IKeyHandler
{
public:
    virtual bool handleKey(int key) = 0;
};

class GotoApplication : public NCursesApplication, public IKeyHandler
{
public:
    bool handleKey(int key)
    {
        switch (key) {
        case 'q':
            exit();
            return true;
        default:
            return false;
        }
    }
};

// --- Menus and Menu Items ----------------------------------------------------------------------

class AbstractMenuItem
{
public:
    virtual ~AbstractMenuItem() {}

    virtual string identifier() const = 0;
    virtual string path() const = 0;
    virtual string pathDisplayed() const { return path(); }

    virtual bool isEmpty() { return identifier().empty() && path().empty(); }
};

typedef shared_ptr<AbstractMenuItem> MenuItemPointer;
typedef vector<MenuItemPointer> MenuItems;

class BookmarkItem : public AbstractMenuItem {
public:
    BookmarkItem(const string name, const string path) : m_name(name), m_path(path) {}
    string identifier() const { return m_name; }
    string path() const { return m_path; }
    string pathDisplayed() const {
        const string homePath = getEnvironmentVariableOrDie("HOME");
        const int homePathLength = homePath.size();
        const string path = this->path();
        if (! path.compare(0, homePathLength, homePath))
            return string(path).replace(0, homePathLength, "~");
        return path;
    }

    string path() { return m_path; }
private:
    const string m_name;
    const string m_path;
};

// TODO: Introduce abstract class
class BookmarkItemVisualHints
{
public:
    BookmarkItemVisualHints(const MenuItemPointer item)
        : color(NCursesApplication::ColorDefault)
        , attributes(0)
    {
        assert(item);

        if (! item->isEmpty()) {
            const string path = item->path();
            Utils::FileInfo fileInfo(path);

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

    NCursesApplication::Color color;
    int attributes;
    string hint;
};

class PathHandlerHint
{
public:
    PathHandlerHint(const string path)
        : hint(NoHandlerHint)
    {
        assert(!path.empty());
        Utils::FileInfo fileInfo(path);
        assert(fileInfo.exists);

        if (fileInfo.isDirectory) {
            hint = ChangeToDirectory;
        } else {
            hint = fileInfo.isExecutable
                ? ExecuteApplication : OpenWithDefaultApplication;
        }
    }

    enum Hint {
        NoHandlerHint,
        ChangeToDirectory,
        ExecuteApplication,
        OpenWithDefaultApplication
    } hint;
};

typedef function<void()> KeyHandlerFunction;
typedef map<int, KeyHandlerFunction> KeyMap;
typedef map<int, KeyHandlerFunction>::iterator KeyMapIterator;

/// Represents the visible lines if there are more menu entries than window lines.
class ScrollView
{
public:
    ScrollView(unsigned firstRow, unsigned rowCount)
        : m_firstRow(firstRow), m_rowCount(rowCount) {}

    unsigned rowCount() { return m_rowCount; }

    unsigned firstRow() { return m_firstRow; }
    unsigned lastRow() { return m_firstRow + m_rowCount - 1; }

    bool isRowBefore(unsigned row) { return row < m_firstRow; }
    bool isRowBehind(unsigned row) { return lastRow() < row; }

    void resetTo(unsigned firstRow) { m_firstRow = firstRow; }

    void moveDown(unsigned times = 1)
    {
        while (times--) {
            ++m_firstRow;
            assert(m_firstRow != std::numeric_limits<unsigned>::max());
        }
    }

    void moveUp(unsigned times = 1)
    {
        while (times--) {
            --m_firstRow;
            assert(m_firstRow != std::numeric_limits<unsigned>::max());
        }
    }

private:
    unsigned m_firstRow;
    unsigned m_rowCount;
};

class StatusBar
{
public:
    StatusBar(int rows, int columns, int beginY, int beginX)
        : m_textAttributes(0)
        , m_window(newwin(rows, columns, beginY, beginX))
    {
    }

    ~StatusBar() { delwin(m_window); }

    void setText(const string &text, int attributes, NCursesApplication::Color color)
    { m_text = text; m_textAttributes = attributes; m_textColor = color; }

    void update() {
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

private:
    string m_text;
    int m_textAttributes;
    NCursesApplication::Color m_textColor;
    WINDOW *m_window;
};

class FilterMenu : public IKeyHandler
{
public:
    FilterMenu(const MenuItems menuItems = MenuItems(), IKeyHandler *parentKeyHandler = 0);
    ~FilterMenu() { delwin(m_window); }

    enum MenuResult { ItemChosen, NoItemChosen };
    int exec();
    MenuItemPointer chosenItem();

    void reset();

    void updateMenu();
    void updateStatusBar();

    void navigateEntryUp();
    void navigateEntryDown();
    void navigatePageUp();
    void navigatePageDown();
    void navigateToStart();
    void navigateToEnd();
    void navigateByDigit();
    void fire();

protected:
    KeyMap m_map;
    MenuItems m_menuItems;

private:
    void printInputSoFar();
    bool handleKey(int key);

    /// When true, jump to the first entry if pressing down arrow on last
    /// item and jump to the last entry if pressing up arrow on first item.
    bool m_optionWrapOnEntryNavigation;

    int m_key;
    MenuItemPointer m_chosenItem;
    string m_filterInput;
    IKeyHandler *m_parentKeyHandler;
    ScrollView m_scrollView;
    unsigned m_selectedRow;
    WINDOW *m_window;
    StatusBar m_statusBar;
};

FilterMenu::FilterMenu(const MenuItems menuItems, IKeyHandler *parentKeyHandler)
    : m_menuItems(menuItems)
    , m_optionWrapOnEntryNavigation(false)
    , m_key(-1)
    , m_chosenItem(0)
    , m_parentKeyHandler(parentKeyHandler)
    , m_scrollView(0, LINES - 2)
    , m_selectedRow(0)
    , m_window(newwin(LINES - 1, COLS, 0, 0))
    , m_statusBar(1, COLS, LINES - 1, 0)
{
    int windowColumns, windowRows;
    getmaxyx(m_window, windowRows, windowColumns);
    m_scrollView = ScrollView(0, windowRows);
    debug() << "FilterMenu: Window size: " << windowColumns << "x" << windowRows;

    keypad(m_window, TRUE);
    m_map[KEY_UP] = bind(&FilterMenu::navigateEntryUp, this);
    m_map['k'] = bind(&FilterMenu::navigateEntryUp, this);
    m_map[KEY_DOWN] = bind(&FilterMenu::navigateEntryDown, this);
    m_map['j'] = bind(&FilterMenu::navigateEntryDown, this);
    m_map[KEY_NPAGE] = bind(&FilterMenu::navigatePageDown, this);
    m_map[KEY_PPAGE] = bind(&FilterMenu::navigatePageUp, this);
    m_map[KEY_HOME] = bind(&FilterMenu::navigateToStart, this);
    m_map[KEY_END] = bind(&FilterMenu::navigateToEnd, this);
    m_map[KEY_RETURN] = bind(&FilterMenu::fire, this);

    for (int i = '0'; i <= '9'; ++i)
        m_map[i] = bind(&FilterMenu::navigateByDigit, this);
}

int FilterMenu::exec()
{
    while (! m_chosenItem) {
        updateMenu();
        updateStatusBar();
        m_key = wgetch(m_window);
        if (! handleKey(m_key) && m_parentKeyHandler)
            m_parentKeyHandler->handleKey(m_key);
    }

    return ItemChosen;
}

MenuItemPointer FilterMenu::chosenItem()
{
    return m_chosenItem;
}

void FilterMenu::reset()
{
    m_selectedRow = 0;
    m_scrollView.resetTo(0);
    // If the last entry is removed from the bookmarks file,
    // the menu is printed in its new dimensions. But the
    // last line is left on the screen. Therefore, clear the
    // window completely instead of doing just a refresh.
    // The refresh will be triggered anyway at printMenu().
    wclear(m_window);
    m_statusBar.update();
}

void FilterMenu::updateMenu()
{
    if (m_menuItems.empty())
        return;

    const int x = 0;
    int y = 0;

    // Get width of first column
    unsigned firstColumnWidth = 0;
    for (auto v : m_menuItems) {
        unsigned width = v->identifier().size();
        if (width > firstColumnWidth)
            firstColumnWidth = width;
    }

    // Find first visible digit accessor
    const unsigned firstRow = m_scrollView.firstRow();
    unsigned digitAccessor = 0;
    for (unsigned i = 0; i < firstRow; ++i) {
        if (! m_menuItems.at(i)->isEmpty())
            ++digitAccessor;
    }

    // Print them
    unsigned int to = m_menuItems.size() - 1 < m_scrollView.lastRow()
        ? m_menuItems.size() - 1
        : m_scrollView.lastRow();
    for (unsigned i = firstRow; i <= to; ++i, ++y) {
        const MenuItemPointer item = m_menuItems.at(i);
        const bool isCurrentItem = m_selectedRow == i;

        stringstream ss;
        ss << left << setw(firstColumnWidth) << item->identifier();
        const string paddedDisplayText = ss.str();
        string digitAccessorString;
        if (digitAccessor <= 9 && ! item->isEmpty())
            digitAccessorString = to_string(digitAccessor++);

        BookmarkItemVisualHints hints(item);
        int attributes = 0;
        if (isCurrentItem)
            attributes |= A_REVERSE;

        wattrset(m_window, 0);
        wattron(m_window, attributes);

        // Clear line
        mvwhline(m_window, y, x, NCURSES_ACS(' '), 1000); // TODO: Is it OK to use NCURSES_ACS?

        // Write line
        mvwprintw(m_window, y, x, "%2s %s ", digitAccessorString.c_str(),
                  paddedDisplayText.c_str());

        if (! isCurrentItem && NCursesApplication::supportsColors())
            NCursesApplication::useColor(m_window, hints.color);
        attributes |= hints.attributes;
        wattron(m_window, attributes);

        mvwprintw(m_window, y, x + 3 + firstColumnWidth + 1, "%s",
                  item->pathDisplayed().c_str());

        wattroff(m_window, attributes);
    }
}

void FilterMenu::updateStatusBar()
{
    assert(m_selectedRow < m_menuItems.size());
    MenuItemPointer selectedItem = m_menuItems.at(m_selectedRow);

    BookmarkItemVisualHints hints(selectedItem);

    m_statusBar.setText(hints.hint, hints.attributes, hints.color);
    m_statusBar.update();
}

void FilterMenu::navigateToStart()
{
    m_selectedRow = 0;
    m_scrollView.resetTo(0);
}

void FilterMenu::navigateToEnd()
{
    m_selectedRow = m_menuItems.size() - 1;

    if (m_menuItems.size() == 0)
        m_scrollView.resetTo(0);
    else if (m_scrollView.lastRow() < m_selectedRow)
        m_scrollView.resetTo(m_selectedRow - (m_scrollView.rowCount() - 1));
}

void FilterMenu::navigateByDigit()
{
    const unsigned digit = m_key - '0';
    const unsigned lastRow = m_menuItems.size() - 1;

    for (unsigned i = 0, digitCounter = 0; i <= lastRow; ++i) {
        if (m_menuItems.at(i)->isEmpty())
            continue;

        if (digitCounter == digit) {
            const unsigned newRow = i;
            m_selectedRow = newRow <= lastRow ? newRow : lastRow;
            if (m_scrollView.isRowBefore(m_selectedRow)) {
                m_scrollView.resetTo(m_selectedRow);
            } else if (m_scrollView.isRowBehind(m_selectedRow)) {
                const unsigned countFollowingEntries = lastRow - m_selectedRow;
                const unsigned scrollViewRowCount = m_scrollView.rowCount();
                const unsigned newFirstRowOfScrollView = countFollowingEntries >= scrollViewRowCount
                    ? m_selectedRow
                    : m_selectedRow - (scrollViewRowCount - 1 - countFollowingEntries);
                m_scrollView.resetTo(newFirstRowOfScrollView);
            }
            return;
        }

        ++digitCounter;
    }
}

void FilterMenu::navigateEntryUp()
{
    if (m_selectedRow == 0) {
        if (m_optionWrapOnEntryNavigation)
            navigateToEnd();
    } else {
        const unsigned originalSelectedRow = m_selectedRow;
        while (m_menuItems.at(--m_selectedRow)->isEmpty());

        const bool nonVisibleItemsBefore = m_scrollView.firstRow() != 0;
        const bool selectedLineWouldBeInvisible = m_selectedRow <= m_scrollView.firstRow() - 1;
        if (nonVisibleItemsBefore && selectedLineWouldBeInvisible)
            m_scrollView.moveUp(originalSelectedRow - m_selectedRow);
    }
}

void FilterMenu::navigateEntryDown()
{
    if (m_selectedRow == m_menuItems.size() - 1) {
        if (m_optionWrapOnEntryNavigation)
            navigateToStart();
    } else {
        const unsigned originalSelectedRow = m_selectedRow;
        while (m_menuItems.at(++m_selectedRow)->isEmpty());

        const bool nonVisibleItemsFollowing = m_scrollView.lastRow() < m_menuItems.size() - 1;
        const bool selectedLineWouldBeInvisible = m_selectedRow >= m_scrollView.lastRow() + 1;
        if (nonVisibleItemsFollowing && selectedLineWouldBeInvisible)
            m_scrollView.moveDown(m_selectedRow - originalSelectedRow);
    }

    wrefresh(m_window);
}

void FilterMenu::navigatePageUp()
{
    if (m_selectedRow == 0) {
        assert(m_selectedRow == m_scrollView.firstRow())
        return;
    }

    const int newFirstRow = m_scrollView.firstRow() - m_scrollView.rowCount();
    if (newFirstRow > 0) {
        m_selectedRow = newFirstRow;
        m_scrollView.resetTo(newFirstRow);
    } else {
        navigateToStart();
    }
}

void FilterMenu::navigatePageDown()
{
    if (m_selectedRow == m_menuItems.size() - 1) {
        assert(m_selectedRow == m_scrollView.lastRow())
        return;
    }

    const unsigned newFirstRow = m_scrollView.lastRow() + 1;
    const unsigned newLastRow = newFirstRow + m_scrollView.rowCount() - 1;
    if (newLastRow < m_menuItems.size() - 1) {
        m_selectedRow = newFirstRow;
        m_scrollView.resetTo(newFirstRow);
    } else {
        navigateToEnd();
    }
}

void FilterMenu::fire()
{
    assert(m_selectedRow <= m_menuItems.size() - 1);
    m_chosenItem = m_menuItems.at(m_selectedRow);
}

void FilterMenu::printInputSoFar()
{
    mvwprintw(m_window, 1, 1, "Filter: '%s'", m_filterInput.c_str());
    wrefresh(m_window);
    // wclear(m_menu_win); // This one flickers with urxvt.
}

bool FilterMenu::handleKey(int key)
{
    KeyMapIterator it = m_map.find(key);
    if (it == m_map.end())
        return false;

    KeyHandlerFunction handler = it->second;
    (handler)();
    return true;
}

typedef shared_ptr<BookmarkItem> BookmarkItemPointer;

class BookmarkMenu : public FilterMenu
{
public:
    BookmarkMenu(const MenuItems menuItems = MenuItems(), IKeyHandler *parentKeyHandler = 0);
    void openEditor();

    BookmarkItemPointer chosenItem();
private:
    void readBookmarksFromFile();
};

BookmarkMenu::BookmarkMenu(const MenuItems menuItems, IKeyHandler *parentKeyHandler)
    : FilterMenu(menuItems, parentKeyHandler)
{
    readBookmarksFromFile();
    m_map['e'] = bind(&BookmarkMenu::openEditor, this);
}

void BookmarkMenu::openEditor()
{
    stringstream command;
    command << "$EDITOR " << "$HOME/" << BookmarkFile;

    NCursesApplication::runExternalCommand(command.str());
    readBookmarksFromFile();
    reset(); // Cursor might be on the last entry and the user might deleted the last entry.
}

BookmarkItemPointer BookmarkMenu::chosenItem()
{
    return static_pointer_cast<BookmarkItem>(FilterMenu::chosenItem());
}

// --- Input and Output --------------------------------------------------------------------------

void BookmarkMenu::readBookmarksFromFile()
{
    const string homePath = getEnvironmentVariableOrDie("HOME");
    const string filePath = homePath + string("/") + string(BookmarkFile);

    // Read file
    ifstream file(filePath.c_str());
    if (! file)
        NCursesApplication::error("Could not open file \"" + filePath + "\"");

    // Parse file
    m_menuItems.clear();
    string line;
    const char delimiter = ',';
    bool lastLineWasEmpty = false;
    for (unsigned lineNumber = 1; file && getline(file, line); ++lineNumber) {
        // Merge multiple empty lines to one entry
        string copiedLine(line);
        const bool isEmptyLine = Utils::trim(copiedLine).empty();
        if (lastLineWasEmpty) {
            if (isEmptyLine)
                continue;
            else
                lastLineWasEmpty = false;
        } else {
            if (isEmptyLine)
                lastLineWasEmpty = true;
        }

        // Parse line
        string bookmarkName;
        string bookmarkPath;
        if (! isEmptyLine) {
            istringstream lineStream(line);
            const bool gotBookmarkName = getline(lineStream, bookmarkName, delimiter);
            const bool gotBookmarkPath = getline(lineStream, bookmarkPath, delimiter);
            if (! gotBookmarkName || ! gotBookmarkPath) {
                const string reason = "Malformed line " + to_string(lineNumber) + " in " + filePath;
                NCursesApplication::error(reason);
            }
        }

        Utils::rtrim(bookmarkName); // Don't trim in the beginning. User might want to indent.
        Utils::trim(bookmarkPath);

        m_menuItems.push_back(BookmarkItemPointer(new BookmarkItem(bookmarkName, bookmarkPath)));
    }

    // Discard only line or last line if it is empty.
    if (! m_menuItems.empty() && m_menuItems.back()->isEmpty())
            m_menuItems.pop_back();

    file.close();
}

static void writeFile(const string filePath, const string fileContents)
{
    ofstream file(filePath);
    if (! file)
        NCursesApplication::error("Could not open file \"" + filePath + "\" for writing");
    file << fileContents << flush;
    if (! file)
        NCursesApplication::error("Failed to write file  \"" + filePath + "\"");
}

// --- Main --------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    GotoApplication app;

    BookmarkMenu menu(MenuItems(), &app);
    menu.exec(); // Block until the user decided for an item.

    BookmarkItemPointer item = menu.chosenItem();
    assert(item);
    PathHandlerHint handlerHint(item->path());
    assert(handlerHint.hint != PathHandlerHint::NoHandlerHint);

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
        case PathHandlerHint::ChangeToDirectory:
            fileContents = "cd \"" + item->path() + '"';
            break;
        case PathHandlerHint::ExecuteApplication:
            fileContents = '"' + item->path() + '"';
            break;
        case PathHandlerHint::OpenWithDefaultApplication:
            fileContents = "xdg-open \"" + item->path() + '"';
            break;
        default:
            fileContents = "Ops, could not determine command to handle path \""
                + item->path() + "\".";
        }
    }

    // No result file is written if the user aborts by e.g. Ctrl-C since we
    // never will get to this point.  This is OK since the shell function
    // handles this case.
    const string filePath = getEnvironmentVariableOrDie("HOME") + "/" + ResultFile;
    writeFile(filePath, fileContents);

    return EXIT_SUCCESS;
}
