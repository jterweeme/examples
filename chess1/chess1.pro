TEMPLATE = app
CONFIG += silent
#QMAKE_POST_LINK=copy gnuchess.boo $$DESTDIR

SOURCES += about.cpp\
    board.cpp\
    book.cpp\
    colordlg.cpp\
    dsp.cpp\
    globals.cpp\
    hittest.cpp\
    main.cpp\
    mainwin.cpp\
    manual.cpp\
    mswdsp.cpp\
    numdlg.cpp\
    palette.cpp\
    piece.cpp\
    promote.cpp\
    review.cpp\
    sim.cpp\
    stats.cpp\
    test.cpp\
    timecnt.cpp\
    toolbox.cpp\
    winclass.cpp

HEADERS += board.h\
    book.h\
    chess.h\
    colordlg.h\
    dialog.h\
    globals.h\
    hittest.h\
    mainwin.h\
    manual.h\
    palette.h\
    promote.h\
    protos.h\
    resource.h\
    review.h\
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

DEFINES -= UNICODE _UNICODE
LIBS += -lgdi32
