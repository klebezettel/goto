TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -pedantic -std=c++11

include(core/core.pri)
include(utils/utils.pri)
include(gui-ncurses/gui-ncurses.pri)

SOURCES += \
    goto.cpp \
    gotoapplication.cpp

HEADERS += \
    gotoapplication.h

OTHER_FILES += README.md

unix {
    debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
    release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
    debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
    release:MOC_DIR = $${OUT_PWD}/.moc/release-shared
}
