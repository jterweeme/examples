#include "packet.h"
#include "input.h"

Packet::Packet()
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

    if (ret != FULLSIZE)
        return -1;

    if (header() == EOT)
        return EOT;

    if (header() == ETB)
        return ETB;

    for (int i = 0; i < SIZE; ++i)
        _data[i] = _buf[i + 3];

    return SOH;
}

void Packet::writeData(std::ostream &os) const
{
    os.write(_data, SIZE);
}
