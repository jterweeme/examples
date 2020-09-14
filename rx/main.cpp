#include "input.h"
#include "xrecv.h"
#include "logger.h"
#include <fstream>
#include <termios.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Enter target filename";
        std::cerr.flush();
        return -1;
    }

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

    std::ofstream logfile;
    logfile.open("xrecv.log", std::ofstream::out | std::ofstream::app);
    Logger logger(&logfile);
    logger.log("Startup...");

    InputStream is(0, &logger);
    is.init();

    XReceiver xReceiver(&is, &std::cout, &logger);
    std::ofstream ofs;
    ofs.open(argv[1]);
    xReceiver.receive(ofs);
    ofs.close();

    logger.log("Shutting down...");
    logfile.close();

    //restore terminal
    tcsetattr(0, TCSADRAIN, &old_tty);
    return 0;
}

