#include "logger.h"
#include <cstdarg>

#ifdef WIN32
#include <windows.h>
#endif

Logger::Logger(std::ostream *os) : _os(os)
{

}

void Logger::log(const char *s)
{
#ifdef WIN32
    char buf[100];
    GetTimeFormatA(LOCALE_USER_DEFAULT, 0, NULL, NULL, buf, 100);
    *_os << buf << " " << s << std::endl;
#else
    *_os << s << "\r\n";
#endif
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
