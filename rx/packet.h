#ifndef PACKET_H
#define PACKET_H

#include <iostream>

class InputStream;

class Logger;

class Packet
{
private:
    static constexpr uint16_t SIZE = 128;
    static constexpr uint16_t FULLSIZE = 128 + 5;
    Logger *_log;
    char _buf[FULLSIZE];
    char _data[SIZE];
    uint16_t _crc;
public:
    static constexpr uint8_t SOH = 1, EOT = 4, ETB = 0x17;

    Packet(Logger *log);
    virtual ~Packet();
    uint8_t volgnummer() const;
    uint8_t header() const;
    int read(InputStream *is, int timeout);
    void writeData(std::ostream &os) const;
};

#endif

