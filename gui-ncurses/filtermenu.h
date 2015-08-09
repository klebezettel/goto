#ifndef FILTERMENU_H
#define FILTERMENU_H

#include "ikeyhandler.h"
#include "scrollview.h"
#include "statusbar.h"

#include "core/imenuitem.h"
#include "core/imodel.h"

#include <functional>
#include <map>
#include <string>

#include <ncurses.h>

namespace TUI {
namespace NCurses {

class FilterMenu : public IKeyController
{
public:
    FilterMenu(Core::IModel &model, IKeyController *parentKeyHandler = 0);
    ~FilterMenu() { delwin(m_window); }

    enum MenuResult { ItemChosen, NoItemChosen };
    int exec();
    MenuItemPointer chosenItem();

    void reset();

    void updateMenu();
    void updateStatusBar();

    bool navigateEntryUp();
    bool navigateEntryDown();
    bool navigatePageUp();
    bool navigatePageDown();
    bool navigateToStart();
    bool navigateToEnd();
    bool navigateByDigit();

    bool fire();

    bool appendToFilter();
    bool chopFromFilter();
    bool clearFilter();

protected:
    using KeyHandlerFunction = std::function<bool()>;
    using KeyMap = std::map<IKeyController::KeyPress, KeyHandlerFunction>;
    using KeyMapIterator = std::map<IKeyController::KeyPress, KeyHandlerFunction>::iterator;

    Core::IModel &m_model;
    KeyMap m_map;
    MenuItems m_allMenuItems;
    MenuItems m_menuItems; // Currently filtered menu items

private:
    void printInputSoFar();
    bool handleKey(KeyPress keyPress);
    void onFilterStringUpdated();

    /// When true, jump to the first entry if pressing down arrow on last
    /// item and jump to the last entry if pressing up arrow on first item.
    bool m_optionWrapOnEntryNavigation;

    int m_key;
    MenuItemPointer m_chosenItem;
    std::string m_filterInput;
    IKeyController *m_parentKeyHandler;
    ScrollView m_scrollView;
    unsigned m_selectedRow;
    WINDOW *m_window;
    StatusBar m_statusBar;
};

} // namespace NCurses
} // namespace TUI

#endif // FILTERMENU_H
