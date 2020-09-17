#include "alarm.h"

#ifdef __linux__
#include <unistd.h>
#include <signal.h>
#endif

Alarm::Alarm()
{

}

void Alarm::callback(int)
{

}

void Alarm::init()
{
#ifdef __linux__
    siginterrupt(SIGALRM, 1);
    signal(SIGALRM, callback);
#endif
}

void Alarm::set(int sec)
{
    (void)sec;
#ifdef __linux
    ::alarm(sec);
#endif
}

