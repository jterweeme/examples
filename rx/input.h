#ifndef INPUT_H
#define INPUT_H

#include "alarm.h"
#include <stddef.h>

class Logger;

class InputStream
{
private:
    Alarm _alarm;
    int _fd;
    Logger *_log;
public:
    InputStream(int fd, Logger *log);
    void init();
    int getc(int timeout);
    int get(char *buf, size_t n, int timeout);
};

#endif

