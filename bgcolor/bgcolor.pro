TEMPLATE = app
CONFIG += silent
LIBS += -lgdi32 -luser32
SOURCES += main.cpp menubar.cpp
HEADERS += menubar.h resource.h
RC_FILE += resource.rc
OTHER_FILES += resource.rc

