TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -pedantic -std=c++11

LIBS += -lncurses

SOURCES += goto.cpp

unix {
	debug:OBJECTS_DIR = $${OUT_PWD}/.obj/debug-shared
	release:OBJECTS_DIR = $${OUT_PWD}/.obj/release-shared
	debug:MOC_DIR = $${OUT_PWD}/.moc/debug-shared
	release:MOC_DIR = $${OUT_PWD}/.moc/release-shared
}
