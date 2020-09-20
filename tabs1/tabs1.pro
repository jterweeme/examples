TEMPLATE = app
CONFIG += silent
LIBS += -lcomctl32 -luser32

SOURCES += element.cpp\
    main.cpp\
    mainwin.cpp\
    tabcontrol.cpp\
    toolbox.cpp\
    winclass.cpp\
    window.cpp

HEADERS += element.h\
    mainwin.h\
    resource.h\
    tabcontrol.h\
    toolbox.h\
    winclass.h\
    window.h

RC_FILE += resource.rc
OTHER_FILES += resource.rc
