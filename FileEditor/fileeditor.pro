#This is a comment
#I love comments

TARGET = FileEditor
TEMPLATE = app
LIBS += -luser32 -lcomdlg32 -lgdi32
SOURCES += editbox.cpp element.cpp main.cpp mainwin.cpp winclass.cpp window.cpp
HEADERS += editbox.h element.h main.h mainwin.h winclass.h window.h
RC_FILE += resource.rc
OTHER_FILES += resource.rc

