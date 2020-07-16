TEMPLATE = app
SOURCES += main.cpp winclass.cpp mainwin.cpp
HEADERS += mdi_unit.h winclass.h mainwin.h main.h
LIBS += -luser32 -lcomdlg32 -lcomctl32 -lgdi32
RC_FILE += mdi_res.rc

