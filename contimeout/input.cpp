#include "input.h"

#ifdef WIN32
InputStreamWin::InputStreamWin(DWORD fd) : _fd(fd)
{

}

void InputStreamWin::init()
{
    _handle = GetStdHandle(_fd);
    GetConsoleMode(_handle, &_oldMode);
    SetConsoleMode(_handle, ENABLE_ECHO_INPUT);
}

BOOL InputStreamWin::thereIsCharEvents() const
{
    INPUT_RECORD tInput[255];
    DWORD dwEvents;

    if (PeekConsoleInput(_handle, tInput, 256, &dwEvents) == 0)
        return TRUE;

    if (dwEvents == 0)
        return TRUE;

    for (DWORD i = 0; i < dwEvents; i++)
    {
        if (tInput[i].EventType != KEY_EVENT)
            continue;

        if (tInput[i].Event.KeyEvent.bKeyDown == FALSE)
            continue;

        if (tInput[i].Event.KeyEvent.uChar.AsciiChar)
            return TRUE;
    }

    ReadConsoleInput(_handle, tInput, dwEvents, &dwEvents);
    return FALSE;
}

/*!
 * \brief InputStreamWin::get
 * \param buf   target buffer
 * \param n     number of bytes to read
 * \param timeout
 * \return
 */
int InputStreamWin::get(char *buf, size_t n, int timeout)
{
    for (size_t i = 0; i < n; ++i)
    {
        int c = getc(timeout);

        if (c == -1)
            return -1;

        buf[i] = c;
    }
    return n;
}

/*!
 * \brief InputStreamWin::getc
 * \param timeout timeout in seconds
 * \return
 */
int InputStreamWin::getc(int timeout)
{
    int c = 0;

    while (true)
    {
        DWORD ret = WaitForSingleObject(_handle, timeout * 1000);

        if (ret == WAIT_OBJECT_0)
        {
            if (thereIsCharEvents() == FALSE)
                continue;

            char buf;
            DWORD read = 0;

            if (ReadFile(_handle, &buf, 1, &read, NULL) == FALSE)
                return -1;

            if (read == 0)
                continue;

            c = buf;

            if (buf == 13)
                buf = 10;

            return c;
        }

        return -1;
    }

    return -1;
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
    size_t ret = ::read(_fd, buf, n);
    _alarm.set(0);
    return ret;
}

int InputStreamUnix::getc(int timeout)
{
    _alarm.set(timeout);
    char c;
    size_t ret = ::read(_fd, &c, 1);
    _alarm.set(0);
    return ret > 0 ? c : -1;
}
#endif
