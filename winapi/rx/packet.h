#ifndef PACKET_H
#define PACKET_H

#include "toolbox.h"

class InputStream;
class Logger;

class Packet
{
private:
    static CONSTEXPR uint16_t SIZE = 128;
    static CONSTEXPR uint16_t FULLSIZE = 128 + 5;
    Logger *_log;
    char _buf[FULLSIZE];
    uint16_t _crc;
    void _debug(int ret);
public:
    static CONSTEXPR uint8_t SOH = 1, EOT = 4, ETB = 0x17;

    Packet(Logger *log);
    virtual ~Packet();
    uint8_t volgnummer() const;
    uint8_t header() const;
    int read(InputStream *is, int timeout);
    void writeData(std::ostream &os) const;
    bool check() const;
};

#endif

