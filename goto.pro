TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -pedantic -std=c++11

LIBS += -lncurses

SOURCES += \
	goto.cpp \
	core/bookmarkitemsmodel.cpp \
	utils/debugutils.cpp \
	utils/fileutils.cpp \
	utils/stringutils.cpp \
	gui-ncurses/bookmarkmenu.cpp \
	gui-ncurses/filtermenu.cpp \
	gui-ncurses/ikeyhandler.cpp \
	gui-ncurses/menuitemvisualhints.cpp \
	gui-ncurses/ncursesapplication.cpp \
	gui-ncurses/scrollview.cpp \
	gui-ncurses/statusbar.cpp \

HEADERS += \
	core/imenuitem.h \
	core/imodel.h \
	core/bookmarkitemsmodel.h \
	utils/debugutils.h\
	utils/fileutils.h \
    utils/stringutils.h \
	gui-ncurses/bookmarkmenu.h \
	gui-ncurses/filtermenu.h \
	gui-ncurses/ikeyhandler.h \
	gui-ncurses/menuitemvisualhints.h \
	gui-ncurses/ncursesapplication.h \
	gui-ncurses/scrollview.h \
	gui-ncurses/statusbar.h \

OTHER_FILES += README.md

unix {
	debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
	release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
	debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
	release:MOC_DIR = $${OUT_PWD}/.moc/release-shared
}
