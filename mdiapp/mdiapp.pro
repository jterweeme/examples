TEMPLATE = app
SOURCES += element.cpp main.cpp winclass.cpp mainwin.cpp toolbar.cpp
HEADERS += element.h mdi_unit.h winclass.h mainwin.h main.h toolbar.h
LIBS += -luser32 -lcomdlg32 -lcomctl32 -lgdi32
RC_FILE += mdi_res.rc
OTHER_FILES += mdi_res.rc
