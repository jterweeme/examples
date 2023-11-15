#include <fstream>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <iomanip>

class BitInputStream
{
    std::istream &_is;
    uint32_t _bitBuffer = 0, _bitCount = 0;
public:
    BitInputStream(std::istream &is) : _is(is) { }
    bool readBool() { return readBits(1) == 1; }
    uint32_t readUInt32() { return readBits(16) << 16 | readBits(16); }
    uint32_t readUnary() { uint32_t u = 0; while (readBool()) ++u; return u; }

    uint32_t readBits(uint32_t count)
    {
        if (count > 24)
            throw "Maximaal 24 bits";

        for (; _bitCount < count; _bitCount += 8)
            _bitBuffer = _bitBuffer << 8 | _is.get();

        _bitCount -= count;
        return _bitBuffer >> _bitCount & (1 << count) - 1;
    }
};

class MoveToFront
{
    uint8_t _buf[256];
public:
    MoveToFront() { std::iota(_buf, _buf + 256, 0); }
    uint8_t indexToFront(uint8_t i) { std::rotate(_buf, _buf + i, _buf + i + 1); return _buf[0]; }
};

class CRC32
{
    uint32_t _table[256];
    uint32_t _crc = 0xffffffff;
public:
    void update(uint8_t c) { _crc = _crc << 8 ^ _table[(_crc >> 24 ^ c) & 0xff]; }
    uint32_t crc() const { return ~_crc; }

    CRC32()
    {
        //https://github.com/gcc-mirror/gcc/blob/master/libiberty/crc32.c
        for (uint32_t i = 0, j, c; i < 256; ++i)
        {
            for (c = i << 24, j = 8; j > 0; --j)
                c = c & 0x80000000 ? c << 1 ^ 0x04c11db7 : c << 1;
            _table[i] = c;
        }
    }
};

class Block
{
    CRC32 _crc;
    uint32_t _blockCRC;
    uint32_t *_merged = nullptr;
    int32_t _last = -1;
    uint32_t _repeat = 0, _length = 0, _dec = 0, _curp = 0, _acc = 0;
    uint8_t _nextByte();
    int _read();
    void _init(BitInputStream &bi, uint32_t blockSize);
    ~Block() { delete[] _merged; }
public:
    static uint32_t process(BitInputStream &bi, uint32_t blockSize, std::ostream &os);
};

uint8_t Block::_nextByte()
{       
    uint8_t ret = _curp & 0xff;
    _curp = _merged[_curp >> 8];
    ++_dec;
    return ret;
}

#if 1
int Block::_read()
{       
    while (_repeat < 1)
    {           
        if (_dec == _length)
            return -1;

        uint8_t nextByte = _nextByte();

        if (nextByte != _last)
        {
            _last = nextByte, _repeat = 1, _acc = 1;
            _crc.update(nextByte);
        }
        else if (++_acc == 4)
        {
            _repeat = _nextByte() + 1, _acc = 0;

            for (uint32_t i = 0; i < _repeat; ++i)
                _crc.update(nextByte);
        }
        else
        {
            _repeat = 1;
            _crc.update(nextByte);
        }
    }

    --_repeat;
    return _last;
}
#else
int Block::_read()
{       
    while (_repeat < 1)
    {           
        if (_dec == _length)
            return -1;

        uint8_t nextByte = _nextByte();

        if (nextByte != _last)
        {
            _last = nextByte, _repeat = 1, _acc = 1;
        }
        else if (++_acc == 4)
        {
            _repeat = _nextByte() + 1, _acc = 0;
        }
        else
        {
            _repeat = 1;
        }
    }

    --_repeat;
    return _last;
}
#endif

void Block::_init(BitInputStream &bi, uint32_t blockSize)
{
    uint32_t bwtByteCounts[256] = {0};
    uint8_t symbolMap[256] = {0};
    _blockCRC = bi.readUInt32();

    if (bi.readBool())
        throw "Randomised blocks not supported.";

    uint32_t bwtStartPointer = bi.readBits(24), symbolCount = 0;

    for (uint16_t i = 0, ranges = bi.readBits(16); i < 16; ++i)
        if ((ranges & 1 << 15 >> i) != 0)
            for (uint32_t j = 0, k = i << 4; j < 16; ++j, ++k)
                if (bi.readBool())
                    symbolMap[symbolCount++] = uint8_t(k);

    uint8_t nTables = bi.readBits(3);
    uint16_t nSelectors = bi.readBits(15);
    uint8_t bwtBlock[blockSize], selectors[nSelectors], mtfValue = 0;
    MoveToFront tableMTF, symbolMTF;
    _length = 0;

    for (uint32_t i = 0; i < nSelectors; ++i)
        selectors[i] = tableMTF.indexToFront(bi.readUnary());

    uint32_t _bases[6][25] = {0};
    uint32_t _limits[6][24] = {0};
    uint32_t _symbols[6][258] = {0};
    uint8_t _minLen[6] = {23,23,23,23,23,23};
    uint8_t _maxLen[6] = {0};

    for (uint8_t t = 0, codeLengths[258] = {0}; t < nTables; ++t)
    {
        for (uint32_t i = 0, pos = 0, c = bi.readBits(5); i <= symbolCount + 1; ++i)
        {
            while (bi.readBool())
                c += bi.readBool() ? -1 : 1;
            codeLengths[pos++] = c;
        }
    
        for (uint32_t i = 0; i < symbolCount + 2; ++i)
            _bases[t][codeLengths[i] + 1]++;

        for (uint32_t i = 1; i < 25; ++i)
            _bases[t][i] += _bases[t][i - 1];

        for (uint32_t i = 0; i < symbolCount + 2; ++i)
        {
            _minLen[t] = std::min(codeLengths[i], _minLen[t]);
            _maxLen[t] = std::max(codeLengths[i], _maxLen[t]);
        }

        for (uint32_t i = _minLen[t], code = 0; i <= _maxLen[t]; ++i)
        {
            uint32_t base = code;
            code += _bases[t][i + 1] - _bases[t][i];
            _bases[t][i] = base - _bases[t][i];
            _limits[t][i] = code - 1;
            code <<= 1;
        }

        for (uint32_t i = 0, minLen = _minLen[t]; minLen <= _maxLen[t]; ++minLen)
            for (uint32_t symbol = 0; symbol < symbolCount + 2; ++symbol)
                if (codeLengths[symbol] == minLen)
                    _symbols[t][i++] = symbol;
    }

    for (uint32_t n = 0, grpIdx = 0, grpPos = 0, inc = 1, curTbl = selectors[0];;)
    {
        if (grpPos++ % 50 == 0)
            curTbl = selectors[grpIdx++];
        
        uint8_t i = _minLen[curTbl];
        uint32_t codeBits = bi.readBits(i);
        uint32_t nextSymbol = 0;
    
        for (;i <= 23; ++i)
        {
            if (codeBits <= _limits[curTbl][i])
            {
                nextSymbol = _symbols[curTbl][codeBits - _bases[curTbl][i]];
                break;
            }
    
            codeBits = codeBits << 1 | bi.readBits(1);
        }

        if (nextSymbol == 0)
        {
            n += inc;
            inc <<= 1;
            continue;
        }

        if (nextSymbol == 1)
        {
            n += inc << 1;
            inc <<= 1;
            continue;
        }

        if (n > 0)
        {
            uint8_t nextByte = symbolMap[mtfValue];
            bwtByteCounts[nextByte] += n;
            ++n, inc = 1;

            while (--n >= 1)
                bwtBlock[_length++] = nextByte;
        }

        //end of block
        if (nextSymbol == symbolCount + 1)
            break;

        mtfValue = symbolMTF.indexToFront(nextSymbol - 1);
        uint8_t nextByte = symbolMap[mtfValue];
        bwtByteCounts[nextByte]++;
        bwtBlock[_length++] = nextByte;
    }

    if (_merged)
        delete[] _merged;
    _merged = new uint32_t[_length];
    uint32_t characterBase[256] = {0};

    for (uint16_t i = 0; i < 255; ++i)
        characterBase[i + 1] = bwtByteCounts[i];

    for (uint16_t i = 2; i <= 255; ++i)
        characterBase[i] += characterBase[i - 1];

    for (uint32_t i = 0; i < _length; ++i)
    {
        uint8_t value = bwtBlock[i] & 0xff;
        _merged[characterBase[value]++] = (i << 8) + value;
    }

    _curp = _merged[bwtStartPointer];
}

uint32_t Block::process(BitInputStream &bis, uint32_t blockSize, std::ostream &os)
{
    Block b;
    b._init(bis, blockSize);

    for (int c; (c = b._read()) != -1;)
        os.put(c);

    if (b._blockCRC != b._crc.crc())
        throw "Block CRC mismatch";

    return b._crc.crc();
}

int main(int argc, char **argv)
{
    std::ostream *os = &std::cout;
    std::ostream *msg = &std::cerr;
    std::istream *is = &std::cin;
    std::ifstream ifs;

    if (argc == 2)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

    BitInputStream bi(*is);
    uint16_t magic = bi.readBits(16);

    if (magic != 0x425a)
        throw "invalid magic";

    bi.readBits(8);
    uint8_t blockSize = bi.readBits(8) - '0';
    uint32_t streamCRC = 0;

    while (true)
    {
        uint32_t marker1 = bi.readBits(24), marker2 = bi.readBits(24);

        if (marker1 == 0x314159 && marker2 == 0x265359)
        {
            uint32_t blockCRC = Block::process(bi, blockSize * 100000, *os);
            streamCRC = (streamCRC << 1 | streamCRC >> 31) ^ blockCRC;
            continue;
        }

        if (marker1 == 0x177245 && marker2 == 0x385090)
        {
            uint32_t crc = bi.readUInt32();

            *msg << "0x" << std::setw(8) << std::setfill('0') << std::hex << crc << " 0x"
                 << std::setw(8) << std::setfill('0') << std::hex << streamCRC << "\r\n";

            break;
        }

        throw "format error!";
    }

    ifs.close();
    return 0;
}

