TEMPLATE = app
CONFIG += console silent file_copies
testFile.files = $$files(whouse.jpg)
testFile.path = $$OUT_PWD/release
COPIES += testFile
SOURCES += main.cpp picojpeg.cpp stb_image.cpp
HEADERS += picojpeg.h stb_image.h

