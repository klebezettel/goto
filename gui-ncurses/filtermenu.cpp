#include "filtermenu.h"

#include "core/imodel.h"

#include "menuitemvisualhints.h"

#include "utils/debugutils.h"
#include "utils/fileutils.h"

#include <iomanip>
#include <functional>
#include <sstream>

using namespace Utils::DebugUtils;

namespace TUI {
namespace NCurses {

FilterMenu::FilterMenu(Core::IModel &model, IKeyController *parentKeyHandler)
    : m_model(model)
    , m_allMenuItems(m_model.items(false))
    , m_menuItems(m_allMenuItems)
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
    m_map[IKeyController::KeyPress(KEY_UP)] = std::bind(&FilterMenu::navigateEntryUp, this);
    m_map[IKeyController::KeyPress(KEY_DOWN)] = std::bind(&FilterMenu::navigateEntryDown, this);
    m_map[IKeyController::KeyPress(KEY_NPAGE)] = std::bind(&FilterMenu::navigatePageDown, this);
    m_map[IKeyController::KeyPress(KEY_PPAGE)] = std::bind(&FilterMenu::navigatePageUp, this);
    m_map[IKeyController::KeyPress(KEY_HOME)] = std::bind(&FilterMenu::navigateToStart, this);
    m_map[IKeyController::KeyPress(KEY_END)] = std::bind(&FilterMenu::navigateToEnd, this);
    m_map[IKeyController::KeyPress(KEY_RETURN)] = std::bind(&FilterMenu::fire, this);

    // Synonyms
    m_map[IKeyController::KeyPress('k', true)] = std::bind(&FilterMenu::navigateEntryUp, this);
    m_map[IKeyController::KeyPress('j', true)] = std::bind(&FilterMenu::navigateEntryDown, this);
    m_map[IKeyController::KeyPress('b', true)] = std::bind(&FilterMenu::navigatePageUp, this);
    m_map[IKeyController::KeyPress('f', true)] = std::bind(&FilterMenu::navigatePageDown, this);

    // Access top entries by pressing Alt-Digit
    for (int i = '0'; i <= '9'; ++i)
        m_map[IKeyController::KeyPress(i, true)] = std::bind(&FilterMenu::navigateByDigit, this);

    // All entered printable characters are added to the filter
    for (int i = 32; i < 256; ++i) {
        if (isprint(i))
            m_map[IKeyController::KeyPress(i)] = std::bind(&FilterMenu::appendToFilter, this);
    }
    m_map[IKeyController::KeyPress(KEY_BACKSPACE)] = std::bind(&FilterMenu::chopFromFilter, this);
    m_map[IKeyController::KeyPress(KEY_CTRL_C)] = std::bind(&FilterMenu::clearFilter, this);
    m_map[IKeyController::KeyPress(KEY_CTRL_D)] = std::bind(&FilterMenu::clearFilter, this);
}

int FilterMenu::exec()
{
    bool isEscapePreceded = false;
    while (! m_chosenItem) {
        updateMenu();
        updateStatusBar();
        m_key = wgetch(m_window);
        if (m_key == KEY_ESC) {
            isEscapePreceded = true;
            continue;
        }

        const IKeyController::KeyPress keyPress(m_key, isEscapePreceded);
        if (isEscapePreceded)
            isEscapePreceded = false;

        debug() << "Key:" << keyPress.key << "escapePreded:" << keyPress.escapePreceded;
        if (! handleKey(keyPress) && m_parentKeyHandler)
            m_parentKeyHandler->handleKey(keyPress);
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
    // The refresh will be triggered anyway at updateMenu().
    wclear(m_window);
    m_statusBar.update();
}

void FilterMenu::updateMenu()
{
    // Clear window
    // wclear() flickers with urxvt. werase() works fine.
    werase(m_window);

    if (m_menuItems.empty())
        return;

    const int x = 0;
    int y = 0;

    // Get width of first column
    // Use m_allMenuItems to determine the width, otherwise the column will be adapted on filtering.
    unsigned firstColumnWidth = 0;
    for (auto v : m_allMenuItems) {
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


        std::string digitAccessorString;
        if (digitAccessor <= 9 && ! item->isEmpty())
            digitAccessorString = std::to_string(digitAccessor++);

        MenuItemVisualHints hints(item);
        int attributes = 0;
        if (isCurrentItem)
            attributes |= A_REVERSE;

        wattrset(m_window, 0);
        wattron(m_window, attributes);

        // Clear line
        // With these extra spaces A_REVERSE will highlight the full line
        mvwhline(m_window, y, x, NCURSES_ACS(' '), 1000); // TODO: Is it OK to use NCURSES_ACS?

        // Construct line
        std::stringstream ss;
        ss << std::right << std::setw(2) << digitAccessorString << ' '
           << std::left << std::setw(firstColumnWidth) << item->identifier() << ' ';
        const std::string outDigitAccessorAndIdentifier = ss.str();

        // Write line
        mvwprintw(m_window, y, x, "%s", outDigitAccessorAndIdentifier.c_str());

        if (! isCurrentItem && NCursesApplication::supportsColors())
            NCursesApplication::useColor(m_window, hints.color);
        attributes |= hints.attributes;
        wattron(m_window, attributes);

        std::string outPath = item->pathDisplayed();
        const int startPosition = x + 3  + firstColumnWidth + 1;
        NCursesApplication::maybeChop(m_window, startPosition, outPath);
        mvwprintw(m_window, y, startPosition, "%s", outPath.c_str());

        wattroff(m_window, attributes);
    }

    wrefresh(m_window);
}

void FilterMenu::updateStatusBar()
{
    std::string text;
    const bool isFilterActive = ! m_filterInput.empty();
    if (isFilterActive)
        text = " Filter: " + m_filterInput + ' ' ;

    int attributes = 0;
    NCursesApplication::Color color = NCursesApplication::ColorDefault;
    if (m_selectedRow < m_menuItems.size()) {
        MenuItemPointer selectedItem = m_menuItems.at(m_selectedRow);
        MenuItemVisualHints hints(selectedItem);
        attributes |= hints.attributes;
        color = hints.color;
        const std::string textToAppend = (isFilterActive ? "| " : "") + hints.hint;
        text += textToAppend;
    }

    m_statusBar.setText(text, attributes, color);
    m_statusBar.update();
}

bool FilterMenu::navigateToStart()
{
    m_selectedRow = 0;
    m_scrollView.resetTo(0);
    return true;
}

bool FilterMenu::navigateToEnd()
{
    if (m_menuItems.empty())
        return true;

    m_selectedRow = m_menuItems.size() - 1;

    if (m_menuItems.size() == 0)
        m_scrollView.resetTo(0);
    else if (m_scrollView.lastRow() < m_selectedRow)
        m_scrollView.resetTo(m_selectedRow - (m_scrollView.rowCount() - 1));

    return true;
}

bool FilterMenu::navigateByDigit()
{
    if (m_menuItems.empty())
        return true;

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
            return true;
        }

        ++digitCounter;
    }

    return true;
}

bool FilterMenu::appendToFilter()
{
    m_filterInput.push_back(static_cast<char>(m_key));
    onFilterStringUpdated();
    return true;
}

bool FilterMenu::chopFromFilter()
{
    if (! m_filterInput.size())
        return true;

    m_filterInput.resize(m_filterInput.size() - 1);
    onFilterStringUpdated();

    return true;
}

bool FilterMenu::clearFilter()
{
    if (m_filterInput.empty())
        return false; // Let's have the parent handler invoked!

    m_filterInput.clear();
    onFilterStringUpdated();

    return true;
}

bool FilterMenu::navigateEntryUp()
{
    if (m_menuItems.empty())
        return true;

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

    return true;
}

bool FilterMenu::navigateEntryDown()
{
    const unsigned menuItemsSize = m_menuItems.size();
    if (menuItemsSize == 0)
        return true;

    if (m_selectedRow == menuItemsSize - 1) {
        if (m_optionWrapOnEntryNavigation)
            navigateToStart();
    } else {
        const unsigned originalSelectedRow = m_selectedRow;
        while (m_menuItems.at(++m_selectedRow)->isEmpty());

        const bool nonVisibleItemsFollowing = m_scrollView.lastRow() < menuItemsSize - 1;
        const bool selectedLineWouldBeInvisible = m_selectedRow >= m_scrollView.lastRow() + 1;
        if (nonVisibleItemsFollowing && selectedLineWouldBeInvisible)
            m_scrollView.moveDown(m_selectedRow - originalSelectedRow);
    }

    return true;
}

bool FilterMenu::navigatePageUp()
{
    if (m_menuItems.empty())
        return true;

    if (m_selectedRow == 0) {
        assert(m_selectedRow == m_scrollView.firstRow())
        return true;
    }

    const int newFirstRow = m_scrollView.firstRow() - m_scrollView.rowCount();
    if (newFirstRow > 0) {
        m_selectedRow = newFirstRow;
        m_scrollView.resetTo(newFirstRow);
    } else {
        navigateToStart();
    }

    return true;
}

bool FilterMenu::navigatePageDown()
{
    if (m_menuItems.empty())
        return true;

    if (m_selectedRow == m_menuItems.size() - 1) {
        assert(m_selectedRow == m_scrollView.lastRow())
        return true;
    }

    const unsigned newFirstRow = m_scrollView.lastRow() + 1;
    const unsigned newLastRow = newFirstRow + m_scrollView.rowCount() - 1;
    if (newLastRow < m_menuItems.size() - 1) {
        m_selectedRow = newFirstRow;
        m_scrollView.resetTo(newFirstRow);
    } else {
        navigateToEnd();
    }

    return true;
}

bool FilterMenu::fire()
{
    if (m_menuItems.empty())
        return true;

    assert(m_selectedRow <= m_menuItems.size() - 1);
    MenuItemPointer item = m_menuItems.at(m_selectedRow);

    if (item->isEmpty())
        return true;
    Utils::FileUtils::FileInfo info(item->path());
    if (! info.exists)
        return true;

    m_chosenItem = item;
    return true;
}

void FilterMenu::printInputSoFar()
{
    mvwprintw(m_window, 1, 1, "Filter: '%s'", m_filterInput.c_str());
    wrefresh(m_window);
    // wclear(m_menu_win); // This one flickers with urxvt.
}

bool FilterMenu::handleKey(KeyPress keyPress)
{
    KeyMapIterator it = m_map.find(keyPress);
    if (it == m_map.end())
        return false;

    KeyHandlerFunction handler = it->second;
    return (handler)();
}

void FilterMenu::onFilterStringUpdated()
{
    m_selectedRow = 0;
    m_scrollView.resetTo(0);

    if (m_filterInput.empty()) {
        m_menuItems = m_allMenuItems;
        return;
    }

    m_menuItems.clear();
    for (unsigned i = 0; i < m_allMenuItems.size(); ++i) {
        MenuItemPointer item = m_allMenuItems.at(i);
        const bool identifierMatches = item->identifier().find(m_filterInput) != std::string::npos;
        const bool pathMatches = item->pathDisplayed().find(m_filterInput) != std::string::npos;
        if (identifierMatches || pathMatches)
            m_menuItems.push_back(item);
    }
}

} // namespace NCurses
} // namespace TUI
