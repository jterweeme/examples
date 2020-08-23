TEMPLATE = app
LIBS += -lcomctl32 -lgdi32 -luser32

SOURCES += element.cpp main.cpp mainwin.cpp toolbox.cpp\
    winclass.cpp window.cpp

HEADERS += element.h mainwin.h property.h resource.h\
    toolbox.h winclass.h window.h

RC_FILE += property.rc
OTHER_FILES += property.rc
