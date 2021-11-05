#include "packet.h"
#include "input.h"
#include "logger.h"
#include "toolbox.h"

Packet::Packet(Logger *log) : _log(log)
{

}

Packet::~Packet()
{

}

uint8_t Packet::volgnummer() const
{
    return _buf[1];
}

uint8_t Packet::header() const
{
    return _buf[0];
}

void Packet::_debug(int ret)
{
    _log->logf("got %d out of %u bytes!", ret, FULLSIZE);

    std::string s;

    for (int i = 0; i < ret; ++i)
    {
        s.append(Toolbox::hex8(_buf[i]));
        s.push_back(' ');
    }

    _log->log(s.c_str());
}

int Packet::read(InputStream *is, int timeout)
{
    int ret = is->get(_buf, FULLSIZE, timeout);
    _debug(ret);
    return ret == FULLSIZE ? ret : -1;
}

void Packet::writeData(std::ostream &os) const
{
    os.write(_buf + 3, SIZE);
}

bool Packet::check() const
{
    uint16_t crc = 0;
    crc |= _buf[131] << 0;
    crc |= _buf[132] << 8;

    uint16_t crc16 = 0;
    
    for (size_t i = 0; i < SIZE; ++i)
    {
        crc16 = crc16 ^ uint16_t(_buf[i + 3]) << 8;

        for (uint8_t j = 0; j < 8; ++j)
            crc16 = crc16 & 0x8000 ? crc16 << 1 ^ 0x1021 : crc << 1;
    }

    std::string scrc = Toolbox::hex16(crc);
    std::string scrc16 = Toolbox::hex16(crc16);
    _log->logf("CRC: %s %s", scrc.c_str(), scrc16.c_str());
    return true;
}

