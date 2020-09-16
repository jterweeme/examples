#include "xrecv.h"
#include "logger.h"
#include "packet.h"

XReceiver::XReceiver(InputStream *is, std::ostream *os, Logger *log)
    : _is(is), _os(os), _log(log)
{

}

void XReceiver::receive(std::ostream &os)
{
    *_os << "waiting to receive.";
    _os->flush();
    Packet packet(_log);

    while (_timeouts < 10)
    {
        //wanneer nog geen packet is ontvangen moet
        //nog een verbonding tot stand worden gebracht
        //door 'C' te sturen. C voor CRC modus
        if (_packets == 0)
        {
            _log->log("C");
            _os->put('C');
            _os->flush();
        }

        _log->logf("start packet.read");
        int ret = packet.read(_is, 5);
        _log->logf("packet.read: %d", ret);

        if (ret == -1)
        {
            //_log->logf("Timeout packet %d!", _packets);
            _timeouts++;
            continue;
        }

        switch (packet.header())
        {
        case Packet::SOH:
            _packets++;
            //_log->logf("Write packet %d %d", _packets, packet.volgnummer());
            packet.writeData(os);
            _os->put(ACK);
            _os->flush();
            break;
        case Packet::EOT:
            return;
        }
    }
}
