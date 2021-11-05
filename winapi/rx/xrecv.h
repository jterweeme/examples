#ifndef XRECV_H
#define XRECV_H

#include "toolbox.h"

class InputStream;
class Logger;

class XReceiver
{
private:
    static CONSTEXPR uint8_t ACK = 6;

    InputStream *_is;
    std::ostream *_os;
    Logger *_log;
    int _packets;
    int _timeouts;
public:
    XReceiver(InputStream *is, std::ostream *os, Logger *log);
    void receive(std::ostream &os);
};

#endif

