TEMPLATE = app
LIBS += -luser32 -lgdi32
SOURCES += main.cpp menubar.cpp toolbox.cpp
HEADERS += menubar.h resource.h toolbox.h
RC_FILE += resource.rc
OTHER_FILES += resource.rc

