#include "input.h"
#include "xrecv.h"
#include "logger.h"
#include <fstream>

#ifdef WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Enter target filename";
        std::cerr.flush();
        return -1;
    }

    std::ofstream logfile;
#if 1
    logfile.open("xrecv.log", std::ofstream::out | std::ofstream::app);
#endif
    Logger logger(&logfile);
    logger.log("Startup...");
#ifdef WIN32
    InputStreamWin is(STD_INPUT_HANDLE, &logger);
    is.init();
#else
    struct termios old_tty, tty;
    tcgetattr(0, &old_tty);
    tty = old_tty;
    tty.c_iflag = IGNBRK;
    tty.c_lflag &= ~(ECHO | ICANON | ISIG);
    tty.c_oflag = 0;
    tty.c_cflag &= ~(PARENB);
    tty.c_cflag &= ~(CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    tcsetattr(0, TCSADRAIN, &tty);
    InputStreamUnix is(0, &logger);
    is.init();
#endif

    XReceiver xReceiver(&is, &std::cout, &logger);
    std::ofstream ofs;
    logger.logf("Opening/creating file %s...", argv[1]);
    ofs.open(argv[1]);
    xReceiver.receive(ofs);
    ofs.close();

    logger.log("Shutting down...");
    logfile.close();

    //restore terminal
#ifndef WIN32
    tcsetattr(0, TCSADRAIN, &old_tty);
#endif
    return 0;
}

