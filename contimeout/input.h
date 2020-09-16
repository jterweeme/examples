#ifndef INPUT_H
#define INPUT_H

#include <windows.h>

class InputStreamWin
{
private:
    DWORD _fd;
    HANDLE _handle;
    DWORD _oldMode;
    BOOL thereIsCharEvents() const;
public:
    InputStreamWin(DWORD fd);
    void init();
    int getc(int timeout);
};

#endif

