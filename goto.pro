TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -pedantic -std=c++11

LIBS += -lncurses

SOURCES += \
	goto.cpp \
	utils/debugutils.cpp \
	utils/fileutils.cpp \
	utils/stringutils.cpp \
	gui-ncurses/ikeyhandler.cpp \
	gui-ncurses/filtermenu.cpp \
    gui-ncurses/scrollview.cpp \
    gui-ncurses/statusbar.cpp \
    gui-ncurses/bookmarkmenu.cpp \
	gui-ncurses/ncursesapplication.cpp \
    gui-ncurses/menuitemvisualhints.cpp \
    core/bookmarkitemsmodel.cpp

HEADERS += \
	utils/debugutils.h\
	utils/fileutils.h \
    utils/stringutils.h \
	gui-ncurses/ikeyhandler.h \
	gui-ncurses/filtermenu.h \
    gui-ncurses/scrollview.h \
    gui-ncurses/statusbar.h \
    gui-ncurses/bookmarkmenu.h \
	gui-ncurses/ncursesapplication.h \
    gui-ncurses/menuitemvisualhints.h \
    core/imodel.h \
    core/bookmarkitemsmodel.h \
    imenuitem.h

OTHER_FILES += README.md

unix {
	debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
	release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
	debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
	release:MOC_DIR = $${OUT_PWD}/.moc/release-shared
}
