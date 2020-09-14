#include "logger.h"
#include <cstdarg>

Logger::Logger(std::ostream *os) : _os(os)
{

}

void Logger::log(const char *s)
{
    *_os << s << "\r\n";
    _os->flush();
}

void Logger::logf(const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    log(buf);
}
