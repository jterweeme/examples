#include "xrecv.h"
#include "logger.h"
#include "packet.h"

XReceiver::XReceiver(InputStream *is, std::ostream *os, Logger *log)
    : _is(is), _os(os), _log(log)
{

}

void XReceiver::receive(std::ostream &os)
{
    *_os << "waiting to receive...";
    _os->flush();
    Packet packet(_log);

    while (_timeouts < 10)
    {
        //wanneer nog geen packet is ontvangen moet
        //nog een verbonding tot stand worden gebracht
        //door 'C' te sturen. C voor CRC modus
        if (_packets == 0)
        {
            _log->log("Trying to connect in CRC mode...");
            _os->put('C');
            _os->flush();
        }

        int ret = packet.read(_is, 5);

        if (ret == -1)
        {
            _log->logf("Timeout %d, packet %d!", _timeouts + 1, _packets);
            _timeouts++;
            continue;
        }

        switch (packet.header())
        {
        case Packet::SOH:
            _packets++;
            packet.check();
            //_log->logf("Write packet %d %d...", _packets, packet.volgnummer());
            packet.writeData(os);
            _os->put(ACK);
            _os->flush();
            break;
        case Packet::EOT:
            _os->put(ACK);
            _os->flush();
            return;
        case Packet::ETB:
            _os->put(ACK);
            _os->flush();
            return;
        }
    }
}


