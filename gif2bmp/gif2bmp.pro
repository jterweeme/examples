TEMPLATE = app
CONFIG += console silent file_copies
testFile.files = $$files(*.gif)
testFile.path = $$OUT_PWD/release
COPIES += testFile
SOURCES += gif.cpp gif2bmp.cpp main.cpp
HEADERS += gif.h gif2bmp.h
