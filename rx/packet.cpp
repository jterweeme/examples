#include "packet.h"
#include "input.h"
#include "logger.h"

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

int Packet::read(InputStream *is, int timeout)
{
    int ret = is->get(_buf, FULLSIZE, timeout);

    _log->logf("Packet::read: %d", ret);

    if (ret != FULLSIZE)
        return -1;

    for (int i = 0; i < SIZE; ++i)
        _data[i] = _buf[i + 3];

    return ret;
}

void Packet::writeData(std::ostream &os) const
{
    os.write(_data, SIZE);
}
