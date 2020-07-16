#This is a comment
#I love comments

TARGET = FileEditor
TEMPLATE = app
LIBS += -luser32 -lcomdlg32 -lgdi32
SOURCES += main.cpp mainwin.cpp winclass.cpp
HEADERS += main.h mainwin.h winclass.h
RC_FILE += FileEditor.rc
OTHER_FILES += FileEditor.rc

