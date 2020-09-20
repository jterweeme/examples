TEMPLATE = app
CONFIG += silent
SOURCES += element.cpp main.cpp mainwin.cpp statusbar.cpp toolbar.cpp winclass.cpp
HEADERS += element.h mdi_unit.h winclass.h mainwin.h main.h statusbar.h toolbar.h
LIBS += -luser32 -lcomdlg32 -lcomctl32 -lgdi32
RC_FILE += mdi_res.rc
OTHER_FILES += mdi_res.rc
