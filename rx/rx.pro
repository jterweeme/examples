TEMPLATE = app
CONFIG += console silent
LIBS += -luser32
HEADERS += alarm.h input.h logger.h packet.h toolbox.h xrecv.h
SOURCES += alarm.cpp input.cpp logger.cpp main.cpp packet.cpp toolbox.cpp xrecv.cpp
