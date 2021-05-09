TEMPLATE = app
CONFIG += console silent file_copies
testFile.files = $$files(keen1.tar.Z)
testFile.path = $$OUT_PWD/release

CONFIG(debug, debug|release) {
    testFile.path = $$OUT_PWD/debug
}

COPIES += testFile
SOURCES += main.cpp

