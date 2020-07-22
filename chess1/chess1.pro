TEMPLATE = app

SOURCES += about.cpp\
    board.cpp\
    book.cpp\
    color.cpp\
    create.cpp\
    dsp.cpp\
    eval.cpp\
    globals.cpp\
    hittest.cpp\
    initiali.cpp\
    initmenu.cpp\
    main.cpp\
    manual.cpp\
    mswdsp.cpp\
    numdlg.cpp\
    piece.cpp\
    promote.cpp\
    review.cpp\
    saveopen.cpp\
    search.cpp\
    stats.cpp\
    test.cpp\
    timecnt.cpp\
    winclass.cpp

HEADERS += chess.h\
    defs.h\
    globals.h\
    gnuchess.h\
    resource.h\
    winclass.h

RC_FILE += chess.rc
OTHER_FILES += chess.rc
DEFINES -= UNICODE _UNICODE
LIBS += -lgdi32
