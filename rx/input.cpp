#include "input.h"
#include <unistd.h>

InputStream::InputStream(int fd, Logger *log)
    : _fd(fd), _log(log)
{

}

void InputStream::init()
{
    _alarm.init();
}

int InputStream::get(char *buf, size_t n, int timeout)
{
    _alarm.set(timeout);
    size_t ret = ::read(_fd, buf, n);
    _alarm.set(0);
    return ret;
}

int InputStream::getc(int timeout)
{
    _alarm.set(timeout);
    char c;
    size_t ret = ::read(_fd, &c, 1);
    _alarm.set(0);
    return ret > 0 ? c : -1;
}

