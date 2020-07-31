TEMPLATE = app

SOURCES += about.cpp\
    board.cpp\
    book.cpp\
    colordlg.cpp\
    dsp.cpp\
    eval.cpp\
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
    search.cpp\
    sim.cpp\
    stats.cpp\
    test.cpp\
    timecnt.cpp\
    toolbox.cpp\
    winclass.cpp

HEADERS += board.h\
    chess.h\
    colordlg.h\
    globals.h\
    mainwin.h\
    palette.h\
    protos.h\
    resource.h\
    sim.h\
    toolbox.h\
    winclass.h

RC_FILE += chess.rc

OTHER_FILES += chess.rc\
    bishop.bmp\
    bishopm.bmp\
    bishopo.bmp\
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
