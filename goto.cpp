// Requirements
//
// Listing Bookmarks mode:
//  <number> <name> <fullpath>
//
// Config file is ~/.hotlist
//
// Keys:
//   Up, k, Down, j:    Move up and down the list.
//   Home, End:         Select first, last item.
//   Enter:             Go to selected directory.
//   Number:            Select item with entered number.
//   e:                 Open editor with bookmarks file.
//   i:                 Enter filter mode. You can enter a pattern
//                      and the filtered list will be shown.
//                      In filter Mode:
//                          Press any printable character to add it to the filter.
//                          ESC aborts filtering.
//   Esc:               Exit, e.g. stay in $CWD (or abort filter in filter mode).
//
//  Display:
//    - Show path abbreviated (~ instead of $HOME).
//    - Highlight not existent dirs (red).
//    - Highlight bookmark if it's $CWD.
//

#include <cctype>
#include <cstdlib>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include <ncurses.h>

using namespace std;

static const int KEY_ESC = 27;
static const int KEY_RETURN = 10;
static const char BookmarkFile[] = ".hotlist";

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

class NCursesApplication
{
public:
    NCursesApplication()
    {
        SCREEN *newScreen = newterm(NULL, stderr, stdin);
        if (newScreen == NULL)
            error("Error: newterm()");
        /*SCREEN *oldScreen = */set_term(newScreen);
        // Don't delete the oldscreen, otherwise newwin() will fail with a seg fault.
//        if (oldScreen == NULL)
//            error("Error: set_term()");
//        delscreen(oldScreen);

        cbreak();
        noecho();
        keypad(stdscr, TRUE); // Enables us to catch arrow presses.
        curs_set(0);
    }

    ~NCursesApplication() { shutdownNCurses(); }

    // Prefer to use destructor, not this one.
    static void shutdownNCurses() { endwin(); }
    static void resumeNCurses() { refresh(); }

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

class ShellDirsApp : public NCursesApplication, public IKeyHandler
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

class IMenuItem
{
public:
    virtual ~IMenuItem() {}
    enum DisplayFlags { Default = 0x0, Attention = 0x1, Highlight = 0x2 };

    virtual string primaryText() const = 0;
    virtual DisplayFlags primaryTextFlags() const = 0;

    virtual string secondaryText() const = 0;
    virtual DisplayFlags secondaryTextFlags() const = 0;
};

typedef shared_ptr<IMenuItem> MenuItemPointer;
typedef vector<MenuItemPointer> MenuItems;

class BookmarkItem : public IMenuItem {
public:
    BookmarkItem(const string name, const string path) : m_name(name), m_path(path) {}
    string primaryText() const { return m_name; }
    string secondaryText() const
    {
        const string homePath = getEnvironmentVariableOrDie("HOME");
        const int homePathLength = homePath.size();
        if (!m_path.compare(0, homePathLength, homePath))
            return string(m_path).replace(0, homePathLength, "~");
        return m_path;
    }
    DisplayFlags primaryTextFlags() const { return IMenuItem::Default; }
    DisplayFlags secondaryTextFlags() const { return IMenuItem::Default; }

    string path() { return m_path; }
private:
    const string m_name;
    const string m_path;
};

typedef function<void()> KeyHandlerFunction;
typedef map<int, KeyHandlerFunction> KeyMap;
typedef map<int, KeyHandlerFunction>::iterator KeyMapIterator;

class FilterMenu : public IKeyHandler
{
public:
    FilterMenu(const MenuItems menuItems = MenuItems(), IKeyHandler *parentKeyHandler = 0);

    enum MenuResult { ItemChosen, NoItemChosen };
    int exec();
    MenuItemPointer chosenItem();

    void printMenu();
    void navigateUp();
    void navigateDown();
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

    int m_key;
    MenuItemPointer m_chosenItem;
    unsigned m_highlighted_row;
    string m_filterInput;
    IKeyHandler *m_parentKeyHandler;
    WINDOW *m_menu_win;
};

FilterMenu::FilterMenu(const MenuItems menuItems, IKeyHandler *parentKeyHandler)
    : m_menuItems(menuItems)
    , m_key(-1)
    , m_chosenItem(0)
    , m_highlighted_row(0)
    , m_parentKeyHandler(parentKeyHandler)
    , m_menu_win(newwin(0, 0, 0, 0)) // Full screen window.
{
    keypad(m_menu_win, TRUE);
    m_map[KEY_UP] = bind(&FilterMenu::navigateUp, this);
    m_map['k'] = bind(&FilterMenu::navigateUp, this);
    m_map[KEY_DOWN] = bind(&FilterMenu::navigateDown, this);
    m_map['j'] = bind(&FilterMenu::navigateDown, this);
    m_map[KEY_HOME] = bind(&FilterMenu::navigateToStart, this);
    m_map[KEY_END] = bind(&FilterMenu::navigateToEnd, this);
    m_map[KEY_RETURN] = bind(&FilterMenu::fire, this);
    for (int i = '0'; i <= '9'; ++i)
        m_map[i] = bind(&FilterMenu::navigateByDigit, this);
}

int FilterMenu::exec()
{
    while (!m_chosenItem) {
        printMenu();
        m_key = wgetch(m_menu_win);
        if (!handleKey(m_key) && m_parentKeyHandler)
            m_parentKeyHandler->handleKey(m_key);
    }

    return ItemChosen;
}

MenuItemPointer FilterMenu::chosenItem()
{
    return m_chosenItem;
}

void FilterMenu::printMenu()
{
    const int x = 0;
    int y = 0;

    // Get width of first column
    unsigned firstColumnWidth = 0;
    for (auto v : m_menuItems) {
        unsigned width = v->primaryText().size();
        if (width > firstColumnWidth)
            firstColumnWidth = width;
    }

    // Print them
    for (unsigned i = 0; i < m_menuItems.size(); ++i, ++y) {
        const MenuItemPointer item = m_menuItems.at(i);
        const bool isCurrentItem = m_highlighted_row == i;
        const bool shouldBeHighlighted = item->primaryTextFlags() & IMenuItem::Highlight;
        const bool shouldStandOut = item->primaryTextFlags() & IMenuItem::Attention;

        stringstream ss;
        ss << left << setw(firstColumnWidth) << item->primaryText();
        const string paddedDisplayText = ss.str();
        const string digitAccessor = i <= 9 ? to_string(i).c_str() : "";
        int attributes = 0;

        if (isCurrentItem)
            attributes |= A_REVERSE;
        if (shouldBeHighlighted)
            attributes |= A_BOLD;
        if (shouldStandOut)
            attributes |= A_UNDERLINE;
        wattron(m_menu_win, attributes);
        mvwprintw(m_menu_win, y, x, "%2s [%s] %s ", digitAccessor.c_str(), paddedDisplayText.c_str(),
                  item->secondaryText().c_str());
        wattroff(m_menu_win, attributes);
    }
    wrefresh(m_menu_win);
}

void FilterMenu::navigateUp()
{
    if (m_highlighted_row == 0)
        m_highlighted_row = m_menuItems.size();
    else
        --m_highlighted_row;
}

void FilterMenu::navigateToStart()
{
    m_highlighted_row = 0;
}

void FilterMenu::navigateToEnd()
{
    m_highlighted_row = m_menuItems.size()-1;
}

void FilterMenu::navigateByDigit()
{
    const unsigned newRow = m_key - '0';
    if (newRow <= m_menuItems.size()-1)
        m_highlighted_row = newRow;
}

void FilterMenu::navigateDown()
{
    if (m_highlighted_row == m_menuItems.size()-1)
        m_highlighted_row = 0;
    else
        ++m_highlighted_row;
}

void FilterMenu::fire()
{
    m_chosenItem = m_menuItems.at(m_highlighted_row);
}

void FilterMenu::printInputSoFar()
{
    mvwprintw(m_menu_win, 1, 1, "Filter: '%s'", m_filterInput.c_str());
    wrefresh(m_menu_win);
    //        wclear(m_menu_win); // This one flickers with urxvt.
}

bool FilterMenu::handleKey(int key)
{
    KeyMapIterator it = m_map.find(key);
    if (it != m_map.end()) {
        KeyHandlerFunction handler = it->second;
        (handler)();
        return true;
    }
    return false;
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
    {
        NCursesApplication::shutdownNCurses();
        system(command.str().c_str());
        NCursesApplication::resumeNCurses();
        refresh();
//        wrefresh(m_menu_win);
    }
    readBookmarksFromFile();
    refresh();
}

BookmarkItemPointer BookmarkMenu::chosenItem()
{
    return static_pointer_cast<BookmarkItem>(FilterMenu::chosenItem());
}

void BookmarkMenu::readBookmarksFromFile()
{
    const string homePath = getEnvironmentVariableOrDie("HOME");
    const string filePath = homePath + string("/") + string(BookmarkFile);

    // Read file
    ifstream file(filePath.c_str());
    if (!file.is_open())
        NCursesApplication::error("Could not open file \"" + filePath + "\"");

    // Parse file
    m_menuItems.clear();
    string line;
    const char delimiter = ',';
    for (unsigned lineNumber = 1; file.good() && getline(file, line); ++lineNumber) {
        // Skip empty lines
        string copiedLine(line);
        if (trim(copiedLine).empty())
            continue;

        // Parse line
        istringstream lineStream(line);
        string bookmarkName;
        string bookmarkPath;
        if (!getline(lineStream, bookmarkName, delimiter))
            NCursesApplication::error("Malformed line " + to_string(lineNumber) + " in " + filePath);
        if (!getline(lineStream, bookmarkPath, delimiter))
            NCursesApplication::error("Malformed line " + to_string(lineNumber) + " in " + filePath);

        rtrim(bookmarkName); // Don't trim in the beginning. User might want to indent.
        trim(bookmarkPath);

        m_menuItems.push_back(BookmarkItemPointer(new BookmarkItem(bookmarkName, bookmarkPath)));
    }

    file.close();
}

int main()
{
    ShellDirsApp app;

    BookmarkMenu menu(MenuItems(), &app);
    int result = menu.exec();
    if (result == FilterMenu::ItemChosen) {
        // Just print the path
        cout << menu.chosenItem()->path();
    }
    return EXIT_SUCCESS;
}
