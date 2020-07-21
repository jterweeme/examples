TEMPLATE = app

SOURCES += board.cpp\
    book.cpp\
    color.cpp\
    create.cpp\
    dsp.cpp\
    eval.cpp\
    globals.cpp\
    hittest.cpp\
    init.cpp\
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
    timecnt.cpp

HEADERS += chess.h\
    color.h defs.h gnuchess.h\
    saveopen.h stats.h\
    timecnt.h

LIBS += -lgdi32
