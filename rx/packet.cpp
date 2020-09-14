#include "packet.h"
#include "input.h"

Packet::Packet()
{

}

uint8_t Packet::volgnummer() const
{
    return _n;
}

int Packet::read(InputStream *is, int timeout)
{
    char buf[FULLSIZE];
    int ret = is->get(buf, FULLSIZE, timeout);

    if (ret != FULLSIZE)
        return -1;

    if (buf[0] == EOT)
        return EOT;

    if (buf[0] == ETB)
        return ETB;

    _n = buf[1];
    _nn = buf[2];

    for (int i = 0; i < SIZE; ++i)
        _data[i] = buf[i + 3];

    return SOH;
}

void Packet::writeData(std::ostream &os) const
{
    os.write(_data, SIZE);
}
