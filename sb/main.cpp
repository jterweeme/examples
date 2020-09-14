#include <iostream>
#include <fstream>

class YSender
{
private:
    static constexpr uint8_t
        SOH = 0x01, STX = 0x02, EOT = 0x04, ACK = 0x06, NAK = 0x15, CAN = 0x18,
        SYNC_TIMEOUT = 30, MAX_RETRY = 30;

    std::istream * const _is;
    std::ostream * const _os;
    uint8_t firstsec, _crcflag = 1;
    char _txbuf[128];
    void _sync();
    int wctx(std::istream &is);
    int putsec(uint8_t sectnum, size_t cseclen);
    int wctxpn(const char *fn, uint32_t filesize, uint16_t time, uint16_t mode);
    size_t filbuf(std::istream &is);
    void calcCRC16(uint16_t &crc, uint8_t c) const;
public:
    YSender(std::istream *is, std::ostream *os);
    void send(std::istream &is, const char *fn, uint32_t filesize, uint16_t time, uint16_t mode);
};

YSender::YSender(std::istream *is, std::ostream *os)
    : _is(is), _os(os)
{

}

void YSender::_sync()
{
    for (int c = 0; (c = _is->get()) != NAK;)
    {
        if (c == 'C')
        {
            _crcflag = 1;
            break;
        }
    }
}

void YSender::calcCRC16(uint16_t &crc, uint8_t c) const
{
    crc = crc ^ (uint16_t)c << 8;

    for (uint8_t i = 0; i < 8; i++)
        crc = crc & 0x8000 ? crc << 1 ^ 0x1021 : crc << 1;
}

int YSender::putsec(uint8_t sectnum, size_t cseclen)
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
            calcCRC16(crc16, *cp);
            checksum += *cp++;
        }

        _os->put(crc16 >> 8 & 0xff);
        _os->put(crc16 & 0xff);
        _os->flush();
        firstch = _is->get();
gotnak:
        switch (firstch)
        {
        case 'C':
        case NAK:
            continue;
        case ACK:
            firstsec = 0;
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

size_t YSender::filbuf(std::istream &is)
{
    is.read(_txbuf, 128);
    return is.gcount();
}

int YSender::wctx(std::istream &is)
{
    int sectnum = 0, attempts = 0, firstch;
    firstsec = 1;

    do
    {
        firstch = _is->get();

        if (firstch == 'X')
            return -1;
    }
    while (firstch != NAK && firstch != 'C' && firstch != 'G');

    while (filbuf(is) > 0)
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

int YSender::wctxpn(const char *fn, uint32_t filesize, uint16_t modtime, uint16_t mode)
{
    char *p, *q;
    _sync();
    q = (char *)0;

    for (p = (char *)fn, q = _txbuf; *p;)
        if ((*q++ = *p++) == '/')
            q = _txbuf;

    *q++ = 0;
    p = q;

    while (q < (_txbuf + 128))
        *q++ = 0;

    sprintf(p, "%u %lo %o 0 0 0", (uint32_t)filesize, (long)modtime, mode);
    return 0;
}

void YSender::send(
    std::istream &is, const char *fn, uint32_t filesize,
    uint16_t time, uint16_t mode)
{
    firstsec = 1;
    wctxpn(fn, filesize, time, mode);   // write header to buffer
    putsec(0, 128);                     // send the buffer
    wctx(is);                           // receive the body
    wctxpn("", 0, 0, 0);                // write empty header to buffer
    putsec(0, 128);                     // send the buffer
}

int main(int argc, char **argv)
{
    if (argc < 2)
        return 0;

    std::ifstream ifs;
    ifs.open(argv[1], std::ifstream::in | std::ifstream::binary);
    ifs.seekg(0, std::ios::end);
    size_t filesize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
#if 0
    std::cout << filesize << "\n";
    std::cout.flush();
#endif
#if 1
    YSender ySender(&std::cin, &std::cout);
    ySender.send(ifs, argv[1], filesize, 0, 0);
#endif
    ifs.close();
    return 0;
}

