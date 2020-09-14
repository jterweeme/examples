#include "xrecv.h"
#include "logger.h"
#include "packet.h"

XReceiver::XReceiver(InputStream *is, std::ostream *os, Logger *log)
    : _is(is), _os(os), _log(log)
{

}

void XReceiver::receive(std::ostream &os)
{
    *_os << "rx: ready to receive file\n";
    _os->flush();

    while (_timeouts < 10)
    {
        //wanneer nog geen packet is ontvangen moet
        //nog een verbonding tot stand worden gebracht
        //door 'C' te sturen. C voor CRC modus
        if (_packets == 0)
        {
            _os->put('C');
            _os->flush();
        }

        Packet packet;
        int ret = packet.read(_is, 5);

        if (ret == Packet::SOH)
        {
            _packets++;
            _log->logf("Write packet %d %d", _packets, packet.volgnummer());
            packet.writeData(os);
            _os->put(ACK);
            _os->flush();
        }
        else if (ret == Packet::EOT)
        {
            return;
        }
        else
        {
            _log->logf("Timeout packet %d!", _packets);
            _timeouts++;
        }
    }
}
