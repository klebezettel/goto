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
///  Display:
///    - DONE: Show path abbreviated (~ instead of $HOME).
///    - TODO: Highlight not existent dirs (red).
///    - TODO: Highlight bookmark if it's $CWD.
///
///  TODO: First start screen (for instructions)
///  TODO: Help screen with all short cuts and modes.
///  TODO: -help
///  TODO: man page
///

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

using namespace std;

static const int KEY_ESC = 27;
static const int KEY_RETURN = 10;
static const char BookmarkFile[] = ".goto.bookmarks";
static const char ResultFile[] = ".goto.result";

// --- Debug & Assert -----------------------------------------------------------------------------

#define DEBUG_OUTPUT 0
/// Ncurses apps cannot just print to stdout/stderr, so print to a file
class DebugOutput
{
#if DEBUG_OUTPUT
    ofstream m_file;
#endif
public:
#if DEBUG_OUTPUT
    explicit DebugOutput(const char *filePath)
        : m_file(filePath)
    {
        if (! m_file)
            exit(EXIT_FAILURE);
    }

    ~DebugOutput() { m_file << endl << flush; }
#else
    explicit DebugOutput(const char *) {}
#endif

#if DEBUG_OUTPUT
#define OUT_OPERATOR(type) \
    DebugOutput &operator<<(const type value) \
    { \
        m_file << value << endl << flush; \
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

// TODO: Would be nice to append 'endl' after all '<<' operations are done.
static DebugOutput &debug() {
    static DebugOutput out("/tmp/goto_debug.log");
    return out;
}

/// Just a soft assert
#define assert(condition) \
    if (!(condition)) { debug() << "Assertion failed: " << #condition; }

// --- Utils --------------------------------------------------------------------------------------

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

// --- Application classes ------------------------------------------------------------------------

class NCursesApplication
{
public:
    NCursesApplication()
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE); // Enables us to catch arrow presses.
        curs_set(0);
    }

    ~NCursesApplication() { shutdownNCurses(); }

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

class IMenuItem
{
public:
    virtual ~IMenuItem() {}
    enum DisplayFlags { Default = 0x0, Attention = 0x1, Highlight = 0x2 };

    virtual string identifier() const = 0;
    virtual DisplayFlags identifierTextFlags() const = 0;

    virtual string path() const = 0;
    virtual DisplayFlags pathTextFlags() const = 0;
};

typedef shared_ptr<IMenuItem> MenuItemPointer;
typedef vector<MenuItemPointer> MenuItems;

class BookmarkItem : public IMenuItem {
public:
    BookmarkItem(const string name, const string path) : m_name(name), m_path(path) {}
    string identifier() const { return m_name; }
    string path() const
    {
        const string homePath = getEnvironmentVariableOrDie("HOME");
        const int homePathLength = homePath.size();
        if (! m_path.compare(0, homePathLength, homePath))
            return string(m_path).replace(0, homePathLength, "~");
        return m_path;
    }
    DisplayFlags identifierTextFlags() const { return IMenuItem::Default; }
    DisplayFlags pathTextFlags() const { return IMenuItem::Default; }

    string path() { return m_path; }
private:
    const string m_name;
    const string m_path;
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
    void resetTo(unsigned firstRow) { m_firstRow = firstRow; }

    void moveDown()
    {
        ++m_firstRow;
        assert(m_firstRow != std::numeric_limits<unsigned>::max());
    }

    void moveUp()
    {
        --m_firstRow;
        assert(m_firstRow != std::numeric_limits<unsigned>::max());
    }

private:
    unsigned m_firstRow;
    unsigned m_rowCount;
};

class StatusBar
{
public:
    StatusBar(int rows, int columns, int beginY, int beginX)
        : m_window(newwin(rows, columns, beginY, beginX))
    {
        setText(" Press RETURN to enter the directory ");
        update();
    }

    ~StatusBar() { delwin(m_window); }

    void setText(const string &text) { m_text = text; }

    void update() {
        wattron(m_window, A_BOLD);
        mvwhline(m_window, 0, 0, ACS_HLINE, 1000); // TODO: Is it OK to use NCURSES_ACS?
        mvwprintw(m_window, 0, 3, m_text.c_str());
        wattroff(m_window, A_BOLD);

        wrefresh(m_window);
    }

private:
    string m_text;
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

    void printMenu();
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
    /// item and jump to the last entry if pressing uparrow on first item.
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
        printMenu();
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
    wrefresh(m_window);
    m_statusBar.update();
}

void FilterMenu::printMenu()
{
    const int x = 0;
    int y = 0;

    // Get width of first column
    unsigned firstColumnWidth = 0;
    for (auto v : m_menuItems) {
        unsigned width = v->identifier().size();
        if (width > firstColumnWidth)
            firstColumnWidth = width;
    }

    // Print them
    unsigned int to = m_menuItems.size() - 1 < m_scrollView.lastRow()
        ? m_menuItems.size() - 1
        : m_scrollView.lastRow();
    for (unsigned i = m_scrollView.firstRow(); i <= to; ++i, ++y) {
        const MenuItemPointer item = m_menuItems.at(i);
        const bool isCurrentItem = m_selectedRow == i;
        const bool shouldBeHighlighted = item->identifierTextFlags() & IMenuItem::Highlight;
        const bool shouldStandOut = item->identifierTextFlags() & IMenuItem::Attention;

        stringstream ss;
        ss << left << setw(firstColumnWidth) << item->identifier();
        const string paddedDisplayText = ss.str();
        const string digitAccessor = i <= 9 ? to_string(i).c_str() : "";
        int attributes = 0;

        if (isCurrentItem)
            attributes |= A_REVERSE;
        if (shouldBeHighlighted)
            attributes |= A_BOLD;
        if (shouldStandOut)
            attributes |= A_UNDERLINE;
        wattron(m_window, attributes);
        // Clear line
        mvwhline(m_window, y, x, NCURSES_ACS(' '), 1000); // TODO: Is it OK to use NCURSES_ACS?
        // Write line
        mvwprintw(m_window, y, x, "%2s %s %s ", digitAccessor.c_str(), paddedDisplayText.c_str(),
                  item->path().c_str());
        wattroff(m_window, attributes);
    }
    wrefresh(m_window);
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
    const unsigned newRow = m_key - '0';
    const unsigned lastRow = m_menuItems.size() - 1;
    m_selectedRow = newRow <= lastRow ? newRow : lastRow;
    if (m_selectedRow < m_scrollView.firstRow())
        m_scrollView.resetTo(m_selectedRow);
}

void FilterMenu::navigateEntryUp()
{
    if (m_selectedRow == 0) {
        if (m_optionWrapOnEntryNavigation)
            navigateToEnd();
    } else {
        --m_selectedRow;
        const bool nonVisibleItemsBefore = m_scrollView.firstRow() != 0;
        const bool selectedLineWouldBeInvisible = m_selectedRow == m_scrollView.firstRow() - 1;
        if (nonVisibleItemsBefore && selectedLineWouldBeInvisible)
            m_scrollView.moveUp();
    }
}

void FilterMenu::navigateEntryDown()
{
    if (m_selectedRow == m_menuItems.size() - 1) {
        if (m_optionWrapOnEntryNavigation)
            navigateToStart();
    }  else {
        ++m_selectedRow;
        const bool nonVisibleItemsFollowing = m_scrollView.lastRow() < m_menuItems.size() - 1;
        const bool selectedLineWouldBeInvisible = m_selectedRow == m_scrollView.lastRow() + 1;
        if (nonVisibleItemsFollowing && selectedLineWouldBeInvisible)
            m_scrollView.moveDown();
    }

    wrefresh(m_window);
}

void FilterMenu::navigatePageUp()
{
    if (m_selectedRow == 0) {
        assert(m_selectedRow == m_scrollView.firstRow())
        return;
    }

    int newFirstRow = m_scrollView.firstRow() - m_scrollView.rowCount();
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
    for (unsigned lineNumber = 1; file && getline(file, line); ++lineNumber) {
        // Skip empty lines
        string copiedLine(line);
        if (trim(copiedLine).empty())
            continue;

        // Parse line
        istringstream lineStream(line);
        string bookmarkName;
        string bookmarkPath;
        if (! getline(lineStream, bookmarkName, delimiter))
            NCursesApplication::error("Malformed line " + to_string(lineNumber) + " in " + filePath);
        if (! getline(lineStream, bookmarkPath, delimiter))
            NCursesApplication::error("Malformed line " + to_string(lineNumber) + " in " + filePath);

        rtrim(bookmarkName); // Don't trim in the beginning. User might want to indent.
        trim(bookmarkPath);

        m_menuItems.push_back(BookmarkItemPointer(new BookmarkItem(bookmarkName, bookmarkPath)));
    }

    file.close();
}

static void writeResultToFile(const string resultPath, const string filePath)
{
    ofstream file(filePath);
    if (! file)
        NCursesApplication::error("Could not open file \"" + filePath + "\" for writing");
    file << resultPath << flush;
    if (! file)
        NCursesApplication::error("Failed to write file  \"" + filePath + "\"");
}

// --- Main --------------------------------------------------------------------------------------

int main()
{
    GotoApplication app;

    string resultPath = ".";
    BookmarkMenu menu(MenuItems(), &app);
    if (menu.exec() == FilterMenu::ItemChosen)
        resultPath = menu.chosenItem()->path();

    // No result file is written if the user aborts by e.g. Ctrl-C since we
    // never will get to this point.  This is OK since the shell function
    // handles this case.
    const string resultFilePath = getEnvironmentVariableOrDie("HOME") + "/" + ResultFile;
    writeResultToFile(resultPath, resultFilePath);

    return EXIT_SUCCESS;
}
