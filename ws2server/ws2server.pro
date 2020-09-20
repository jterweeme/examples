TEMPLATE = app
CONFIG += console silent
LIBS += -lws2_32
SOURCES += main.cpp ws2tools.cpp
HEADERS += ws2tools.h

