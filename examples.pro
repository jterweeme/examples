TEMPLATE = subdirs

SUBDIRS = base64\
    bzcat\
    cat\
    chessvalid\
    compress\
    contimeout\
    echosvr\
    exception\
    #gif2bmp\
    grep\
    head\
    jpg2tga\
    #ls\
    lsq\
    md5sum\
    minilang\
    mysql1\
    nl\
    od\
    overlap\
    pack\
    #pcat\
    sb\
    sx\
    tail\
    test1\
    timer1\
    tokenizer\
    wc\
    zcat

win32 | win64 {
    SUBDIRS += winapi
}


