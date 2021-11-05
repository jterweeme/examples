#ifndef ALARM_H
#define ALARM_H

class Alarm
{
private:
    static void callback(int);
public:
    Alarm();
    void init();
    void set(int sec);
};

#endif

