#file: minilang.pro

TEMPLATE = app
CONFIG += silent console

SOURCES += ast.cpp\
    interpreter.cpp\
    lexer.cpp\
    main.cpp\
    parser.cpp\
    semantic_analysis.cpp\
    token.cpp\
    xml_visitor.cpp

HEADERS += ast.h\
    interpreter.h\
    lexer.h\
    semantic_analysis.h\
    table.h\
    token.h\
    visitor.h\
    xml_visitor.h
