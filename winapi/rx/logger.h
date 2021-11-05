#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

class Logger
{
private:
    std::ostream *_os;
public:
    Logger(std::ostream *os);
    void log(const char *s);
    void logf(const char *fmt, ...);
};

#endif

