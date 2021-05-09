TEMPLATE = subdirs

SUBDIRS = base64\
    bzcat\
    cat\
    chessvalid\
    compress\
    contimeout\
    echosvr\
    exception\
    gif2bmp\
    grep\
    head\
    jpg2tga\
    md5sum\
    nl\
    od\
    overlap\
    pack\
    pcat\
    rb\
    sb\
    sx\
    tail\
    wc\
    zcat\
    zeit

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
        rx\
        tabs1\
        tictac\
        winanim\
        winthread\
        ws2client\
        ws2server
}


