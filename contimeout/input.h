#ifndef INPUT_H
#define INPUT_H

#include <windows.h>

class InputStream
{
public:
    virtual int get(char *buf, size_t n, int timeout) = 0;
    virtual int getc(int timeout) = 0;
};

#ifdef WIN32
class InputStreamWin : public InputStream
{
private:
    DWORD _fd;
    HANDLE _handle;
    DWORD _oldMode;
    BOOL thereIsCharEvents() const;
public:
    InputStreamWin(DWORD fd);
    void init();
    int get(char *buf, size_t n, int timeout) override;
    int getc(int timeout) override;
};
#else
class InputStreamUnix : public InputStream
{
private:
    Alarm _alarm;
    int _fd;
    Logger *_log;
public:
    InputStreamUnix(int fd, Logger *log);
    void init();
    int getc(int timeout) override;
    int get(char *buf, size_t n, int timeout) override;
};
#endif

#endif

