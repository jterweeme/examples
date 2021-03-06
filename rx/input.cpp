#include "input.h"
#include "logger.h"
#include "toolbox.h"
#include <iostream>

#ifndef WIN32
#define UNIX
#endif

#ifdef UNIX
#include <unistd.h>
#endif

#ifdef WIN32
InputStreamWin::InputStreamWin(DWORD fd, Logger *log)
    : _fd(fd), _log(log)
{

}

void InputStreamWin::init()
{
    _handle = GetStdHandle(_fd);
    GetConsoleMode(_handle, &_oldMode);
    SetConsoleMode(_handle, 0);
}

BOOL InputStreamWin::thereIsCharEvents() const
{
    CONSTEXPR DWORD MAX_RECORDS = 16;
    INPUT_RECORD tInput[MAX_RECORDS];
    DWORD dwEvents = 0;
#if 0
    BOOL ret = PeekConsoleInputA(_handle, tInput, MAX_RECORDS, &dwEvents);

    if (ret == 0)
        return TRUE;

    if (dwEvents == 0)
        return TRUE;

    for (DWORD i = 0; i < dwEvents; i++)
    {
        if (tInput[i].EventType != KEY_EVENT)
            continue;

        if (tInput[i].Event.KeyEvent.bKeyDown == FALSE)
            continue;

        if (tInput[i].Event.KeyEvent.uChar.AsciiChar || !tInput[i].Event.KeyEvent.wVirtualKeyCode)
            return TRUE;
    }

    _log->logf("ReadConsoleInputA %u events", dwEvents);
    ReadConsoleInputA(_handle, tInput, dwEvents, &dwEvents);
    return FALSE;
#endif
    do
     {
         BOOL ret = PeekConsoleInputA(_handle, tInput, MAX_RECORDS, &dwEvents);

         if (ret == 0)
             return TRUE;

         if (dwEvents == 0)
             return TRUE;

         for (DWORD i = 0; i < dwEvents; i++)
         {
             if (tInput[i].EventType != KEY_EVENT)
                 continue;

             if (tInput[i].Event.KeyEvent.bKeyDown == FALSE)
                 continue;

             if (tInput[i].Event.KeyEvent.wVirtualKeyCode)
                 return TRUE;
#if 0
             if ( (tInput[i].Event.KeyEvent.uChar.AsciiChar) || (!tInput[i].Event.KeyEvent.wVirtualKeyCode) )
                 return TRUE;
#endif
         }

         ReadConsoleInput(_handle, tInput, dwEvents, &dwEvents);
     } while ( dwEvents == MAX_RECORDS );
    return FALSE;
}

int InputStreamWin::getc(int timeout)
{
    int c = 0;

    while (true)
    {
        DWORD ret = WaitForSingleObject(_handle, timeout * 1000);

        if (ret == WAIT_OBJECT_0)
        {
            BOOL tisce = thereIsCharEvents();

            if (tisce == FALSE)
                continue;

            char buf[200];
            DWORD read = 0;
            BOOL rf = ReadFile(_handle, buf, 200, &read, NULL);
            _log->logf("characters read: %u", read);
            std::string s;

            for (DWORD i = 0; i < read; i++)
            {
                s.append(Toolbox::hex8(buf[i]));
            }

            _log->log(s.c_str());

            if (rf == FALSE)
                return -1;

            if (read == 0)
                continue;

            c = buf[0];
#if 0
            if (buf == 13)
                buf = 10;
#endif
            return c;
        }

        return -1;
    }

    return -1;
}

int InputStreamWin::get(char *buf, size_t n, int timeout)
{
    _log->logf("getting %u bytes, timeout %ds...", n, timeout);

    for (size_t i = 0; i < n; ++i)
    {
        int c = getc(timeout);

        if (c == -1)
            return i;

        buf[i] = c;
    }
    return n;
}

InputStreamThread::InputStreamThread(DWORD fd, Logger *log) : _fd(fd), _log(log)
{

}

void InputStreamThread::init()
{

}

DWORD WINAPI testThread(LPVOID lpParam)
{
    int c = std::cin.get();
    int *foo = (int *)lpParam;
    *foo = c;
    return 0;
}

DWORD WINAPI getPacket(LPVOID lpParam)
{
    char *buf = (char *)lpParam;
    std::cin.read(buf, 133);
    return 0;
}

int InputStreamThread::getc(int timeout)
{
    int c = -1;
    HANDLE t = CreateThread(NULL, 0, testThread, &c, 0, 0);
    WaitForSingleObject(t, timeout * 1000);
    return c;
}

int InputStreamThread::get(char *buf, size_t n, int timeout)
{
    _log->logf("getting %u bytes, timeout %ds...", n, timeout);

    if (n == 133)
    {
        HANDLE t = CreateThread(NULL, 0, getPacket, buf, 0, 0);
        WaitForSingleObject(t, timeout * 1000);
        return n;
    }

    for (size_t i = 0; i < n; ++i)
    {
        int c = getc(timeout);

        if (c == -1)
            return i;

        buf[i] = c;
    }

    return n;
}

#endif

#ifdef UNIX
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
    //we moeten bytes individueel lezen
    //anders krijgt serieel niet alle bytes
    //telnet en ssh werkt wel
    for (size_t i = 0; i < n; ++i)
    {
        int c = getc(timeout);

        if (c == -1)
            return i;

        buf[i] = c;
    }

    return n;
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


