#include "alarm.h"
#ifndef WIN32
#include <unistd.h>
#endif
#include <signal.h>

Alarm::Alarm()
{

}

void Alarm::callback(int)
{

}

void Alarm::init()
{
#ifndef WIN32
    siginterrupt(SIGALRM, 1);
    signal(SIGALRM, callback);
#endif
}

void Alarm::set(int sec)
{
    (void)sec;
#ifndef WIN32
    ::alarm(sec);
#endif
}

