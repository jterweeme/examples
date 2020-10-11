#ifndef INPUT_H
#define INPUT_H

#include "alarm.h"
#include <stddef.h>

#ifdef WIN32
#include <windows.h>
#else
#define UNIX
#endif

class Logger;

class InputStream
{
public:
    virtual int get(char *buf, size_t n, int timeout) = 0;
};

#ifdef WIN32
class InputStreamWin : public InputStream
{
private:
    DWORD _fd;
    Logger *_log;
    HANDLE _handle;
    DWORD _oldMode;
    BOOL thereIsCharEvents() const;
public:
    InputStreamWin(DWORD fd, Logger *log);
    void init();
    int getc(int timeout);
    int get(char *buf, size_t n, int timeout) override;
};

class InputStreamThread : public InputStream
{
private:
    DWORD _fd;
    Logger *_log;
public:
    InputStreamThread(DWORD fd, Logger *log);
    void init();
    int getc(int timeout);
    int get(char *buf, size_t n, int timeout) override;
};

#endif

#ifdef UNIX
class InputStreamUnix : public InputStream
{
private:
    Alarm _alarm;
    int _fd;
    Logger *_log;
public:
    InputStreamUnix(int fd, Logger *log);
    void init();
    int getc(int timeout);
    int get(char *buf, size_t n, int timeout) override;
};
#endif


#endif

