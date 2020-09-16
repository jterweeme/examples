#include "input.h"
#include <unistd.h>
#include <iostream>

#ifdef WIN32
InputStreamWin::InputStreamWin(DWORD fd, Logger *log)
    : _fd(fd), _log(log)
{

}

void InputStreamWin::init()
{
    _handle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(_handle, &_oldMode);
    DWORD newMode = _oldMode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(_handle, newMode);
}

int InputStreamWin::getc(int timeout)
{
    FlushConsoleInputBuffer(_handle);
    int c;

    if (WaitForSingleObject(_handle, timeout * 1000) == WAIT_OBJECT_0)
    {
        c = std::cin.get();
    }
    else
    {
        //timeout
    }
    return 0;
}

int InputStreamWin::get(char *buf, size_t n, int timeout)
{
    return 0;
}
#else
InputStreamUnix::InputStreamUnix(int fd, Logger *log)
    : _fd(fd), _log(log)
{

}

void InputStreamUnix::init()
{
    _alarm.init();
}

int InputStreamUnix::get(char *buf, size_t n, int timeout)
{
    _alarm.set(timeout);
    ssize_t ret = ::read(_fd, buf, n);
    _alarm.set(0);
    return ret;
}

int InputStreamUnix::getc(int timeout)
{
    _alarm.set(timeout);
    char c;
    ssize_t ret = ::read(_fd, &c, 1);
    _alarm.set(0);
    return ret > 0 ? c : -1;
}
#endif


