#include "alarm.h"

Alarm::Alarm()
{

}

void Alarm::callback(int)
{

}

void Alarm::init()
{
#ifdef LINUX
    siginterrupt(SIGALRM, 1);
    signal(SIGALRM, callback);
#endif
}

void Alarm::set(int sec)
{
    (void)sec;
#ifdef LINUX
    ::alarm(sec);
#endif
}

