TEMPLATE = app
CONFIG += silent
LIBS += -lgdi32 -lmsimg32
SOURCES += dialog.cpp main.cpp MemDcUsage.cpp
HEADERS += dialog.h MemDcUsage.h
RC_FILE += resource.rc
OTHER_FILES += resource.rc
