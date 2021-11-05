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
    SUBDIRS += bgcolor\
        bitmap\
        cechat\
        chatclient\
        chess1\
        colorbtn\
        commctrl1\
        FileEditor\
        mdiapp\
        memdc\
        netkatd\
        opengl1\
        property1\
        qrcode\
        rb\
        rx\
        tabs1\
        tictac\
        winanim\
        winthread\
        ws2client\
        ws2server\
        zeit
}


