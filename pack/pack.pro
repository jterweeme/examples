TEMPLATE = app
CONFIG += console silent file_copies
testFile.files = $$files(manual.doc)
testFile.path = $$OUT_PWD/release
COPIES += testFile
SOURCES += main.cpp

