TEMPLATE = app
CONFIG += console silent file_copies
testFile.files = $$files(manual.doc.z)
testFile.path = $$OUT_PWD/release

CONFIG(debug, debug|release) {
    testFile.path = $$OUT_PWD/debug
}

COPIES += testFile
SOURCES += main.cpp toolbox.cpp
HEADERS += toolbox.h

