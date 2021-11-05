TEMPLATE = app
CONFIG += silent
LIBS += -lgdi32 -lcomctl32 -luser32
SOURCES += main.cpp mainwin.cpp menubar.cpp toolbox.cpp
HEADERS += mainwin.h menubar.h resource.h toolbox.h
RC_FILE += cechat.rc
OTHER_FILES += cechat.rc

