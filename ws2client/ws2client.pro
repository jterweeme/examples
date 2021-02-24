TEMPLATE = app
CONFIG += console silent
LIBS += -lws2_32 -luser32
SOURCES += main.cpp toolbox.cpp ws2tools.cpp
HEADERS += toolbox.h ws2tools.h

