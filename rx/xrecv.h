#ifndef XRECV_H
#define XRECV_H

#include <iostream>

class InputStream;
class Logger;

class XReceiver
{
private:
    static constexpr uint8_t ACK = 6;

    InputStream *_is;
    std::ostream *_os;
    Logger *_log;
    int _packets = 0;
    int _timeouts = 0;
public:
    XReceiver(InputStream *is, std::ostream *os, Logger *log);
    void receive(std::ostream &os);
};

#endif

