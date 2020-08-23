TEMPLATE = app
CONFIG += silent
DESTDIR = $$OUT_PWD/
DEFINES -= UNICODE _UNICODE
LIBS += -lgdi32 -luser32

copydata.commands = $(COPY_FILE) \"$$PWD\gnuchess.boo\" \"$$OUT_PWD/\"
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

SOURCES += board.cpp\
    book.cpp\
    dialog.cpp\
    globals.cpp\
    hittest.cpp\
    main.cpp\
    mainwin.cpp\
    palette.cpp\
    piece.cpp\
    sim.cpp\
    toolbox.cpp\
    winclass.cpp

HEADERS += board.h\
    book.h\
    chess.h\
    dialog.h\
    globals.h\
    hittest.h\
    mainwin.h\
    palette.h\
    resource.h\
    sim.h\
    toolbox.h\
    winclass.h

RC_FILE += chess.rc

OTHER_FILES += chess.rc\
    bishop.bmp\
    bishopm.bmp\
    bishopo.bmp\
    gnuchess.boo\
    king.bmp\
    kingm.bmp\
    kingo.bmp\
    knight.bmp\
    knightm.bmp\
    knighto.bmp\
    pawn.bmp\
    pawnm.bmp\
    pawno.bmp\
    queen.bmp\
    queenm.bmp\
    queeno.bmp\
    rook.bmp\
    rookm.bmp\
    rooko.bmp
