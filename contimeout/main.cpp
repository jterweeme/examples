#include "input.h"
#include <iostream>

#ifndef WIN32
#include <termios.h>
#endif

int main()
{
#ifdef WIN32
    InputStreamWin is(STD_INPUT_HANDLE);
#else
    struct termios old_tty, tty;
    tcgetattr(0, &old_tty);
    tty = old_tty;
    tty.c_iflag = IGNBRK;
    tty.c_lflag &= ~(ECHO | ICANON | ISIG);
    tty.c_cflag &= ~(PARENB);
    tty.c_cflag &= ~(CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    tcsetattr(0, TCSADRAIN, &tty);
    InputStreamUnix is(0);
#endif
    is.init();

    while (true)
    {
        int c = is.getc(2);
        std::cout << c << "\r\n";
        std::cout.flush();
    }
    
#ifndef WIN32
    tcsetattr(0, TCSADRAIN, &old_tty);
#endif
    // restore console mode when exit
    //SetConsoleMode(hStdIn, oldMode);
    return 0;
}

