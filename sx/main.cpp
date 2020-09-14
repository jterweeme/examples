#include <iostream>
#include <fstream>

class XSender
{
private:
    static constexpr uint8_t
        SOH = 0x01, STX = 0x02, EOT = 0x04, ACK = 0x06, NAK = 0x15, CAN = 0x18,
        SYNC_TIMEOUT = 30, MAX_RETRY = 30;

    uint8_t _mode = 0;
    char _txbuf[128];
    std::istream * const _is;
    std::ostream * const _os;
    int putsec(int sectnum, size_t cseclen);
    size_t _filbuf(std::istream &is);
    void _calcCRC16(uint16_t &crc, uint8_t c) const;
public:
    XSender(std::istream *is, std::ostream *os);
    int send(std::istream &is);
};

XSender::XSender(std::istream *is, std::ostream *os) : _is(is), _os(os)
{

}

size_t XSender::_filbuf(std::istream &is)
{
    is.read(_txbuf, 128);
    return is.gcount();
}

int XSender::putsec(int sectnum, size_t cseclen)
{
    int wcj, firstch = 0;
    char *cp;

    for (uint8_t attempts = 0; attempts <= MAX_RETRY; attempts++)
    {
        _os->put(SOH);
        _os->put(sectnum);
        _os->put(-sectnum - 1);
        int checksum = 0;
        uint16_t crc16 = 0;

        for (wcj = cseclen, cp = _txbuf; --wcj >= 0;)
        {
            _os->put(*cp);
            _calcCRC16(crc16, *cp);
            checksum += *cp++;
        }

        if (_mode == 0)
        {
            _os->put(checksum);
        }
        else
        {
            _os->put(crc16 >> 8 & 0xff);
            _os->put(crc16 & 0xff);
        }
        _os->flush();
        firstch = _is->get();
gotnak:
        switch (firstch)
        {
        case 'C':
            _mode = 1;
            continue;
        case NAK:
            _mode = 0;
            continue;
        case ACK:
            return 0;
        default:
            break;
        }

        while (true)
        {
            firstch = _is->get();

            if (firstch == NAK || firstch == 'C')
                goto gotnak;
        }
    }
    return -1;
}

void XSender::_calcCRC16(uint16_t &crc, uint8_t c) const
{
    crc = crc ^ (uint16_t)c << 8;

    for (uint8_t i = 0; i < 8; i++)
        crc = crc & 0x8000 ? crc << 1 ^ 0x1021 : crc << 1;
}

int XSender::send(std::istream &is)
{
    int sectnum = 0, attempts = 0, firstch;

    do
    {
        firstch = _is->get();

        if (firstch == 'X')
            return -1;
    }
    while (firstch != NAK && firstch != 'C' && firstch != 'G');

    while (_filbuf(is) > 0)
        if (putsec(++sectnum, 128) == -1)
            return -1;

    do
    {
        _os->put(EOT);
        _os->flush();
        ++attempts;
        firstch = _is->get();
    }
    while (firstch != ACK && attempts < MAX_RETRY);

    return attempts == MAX_RETRY ? -1 : 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 0;

    XSender xSender(&std::cin, &std::cout);
    std::ifstream ifs;
    ifs.open(argv[1], std::ifstream::in | std::ifstream::binary);
    xSender.send(ifs);
    ifs.close();
    return 0;
}

