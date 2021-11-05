TEMPLATE = app
CONFIG += silent
SOURCES += ball.cpp main.cpp mainwin.cpp winclass.cpp
HEADERS += ball.h mainwin.h winclass.h
RC_FILE += anim.rc
LIBS += -lgdi32 -luser32
OTHER_FILES += anim.rc

