

TEMPLATE = app
CONFIG += console silent file_copies
testFile.files = $$files(relnotes.ps.bz2)
testFile.path = $$OUT_PWD/debug
COPIES += testFile
SOURCES += bitstream.cpp block.cpp main.cpp stream.cpp table.cpp toolbox.cpp
HEADERS += bitstream.h block.h stream.h table.h toolbox.h

