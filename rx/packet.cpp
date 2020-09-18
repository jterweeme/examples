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
    //_log->logf("got %d out of %u bytes!", ret, FULLSIZE);
    return ret == FULLSIZE ? ret : -1;
}

void Packet::writeData(std::ostream &os) const
{
    os.write(_buf + 3, SIZE);
}
