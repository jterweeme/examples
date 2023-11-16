#include <fstream>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <iomanip>

class BitInputStream
{
    std::istream *_is;
    uint32_t _bitBuffer = 0, _bitCount = 0;
public:
    BitInputStream(std::istream *is) : _is(is) { }
    uint32_t readUInt32() { return readBits(16) << 16 | readBits(16); }

    uint32_t readBits(uint32_t count)
    {
        assert(count <= 24);

        for (; _bitCount < count; _bitCount += 8)
            _bitBuffer = _bitBuffer << 8 | _is->get();

        _bitCount -= count;
        return _bitBuffer >> _bitCount & ((1 << count) - 1);
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

class Table
{
    uint8_t _codeLengths[258];
    uint16_t _pos = 0;
    uint32_t _bases[25] = {0};
    uint32_t _limits[24] = {0};
    uint32_t _symbols[258] = {0};
    uint8_t _minLen = 23;
    uint8_t _maxLen = 0;
public:
    void read(BitInputStream &bis, uint32_t symbolCount);
    uint8_t minLength() const { return _minLen; }
    uint32_t limit(uint8_t i) const { return _limits[i]; }
    uint32_t symbol(uint16_t i) const { return _symbols[i]; }
    uint32_t base(uint8_t i) const { return _bases[i]; }
};

//read the canonical Huffman code lengths for table
void Table::read(BitInputStream &bis, uint32_t symbolCount)
{
    for (uint32_t i = 0, c = bis.readBits(5); i <= symbolCount + 1; ++i)
    {
        while (bis.readBits(1))
            c += bis.readBits(1) ? -1 : 1;
        _codeLengths[_pos++] = c;
    }

    for (uint32_t i = 0; i < symbolCount + 2; ++i)
        _bases[_codeLengths[i] + 1]++;

    for (uint32_t i = 1; i < 25; ++i)
        _bases[i] += _bases[i - 1];

    for (uint32_t i = 0; i < symbolCount + 2; ++i)
    {
        _minLen = std::min(_codeLengths[i], _minLen);
        _maxLen = std::max(_codeLengths[i], _maxLen);
    }

    for (uint32_t i = _minLen, code = 0; i <= _maxLen; ++i)
    {
        uint32_t base = code;
        code += _bases[i + 1] - _bases[i];
        _bases[i] = base - _bases[i];
        _limits[i] = code - 1;
        code <<= 1;
    }

    for (uint32_t i = 0, minLen = _minLen; minLen <= _maxLen; ++minLen)
        for (uint32_t symbol = 0; symbol < symbolCount + 2; ++symbol)
            if (_codeLengths[symbol] == minLen)
                _symbols[i++] = symbol;
}

class Tables
{
    Table _tables[6];
    uint8_t *_selectors;
    uint32_t grpIdx, grpPos, curTbl;
public:
    ~Tables() { delete[] _selectors; }
    void read(BitInputStream &bis, uint32_t symbolCount);
    uint32_t nextSymbol(BitInputStream &bis);
};

void Tables::read(BitInputStream &bis, uint32_t symbolCount)
{
    uint8_t nTables = bis.readBits(3);
    uint16_t nSelectors = bis.readBits(15);
    _selectors = new uint8_t[nSelectors];
    MoveToFront tableMTF;
    
    for (uint32_t i = 0; i < nSelectors; ++i)
    {
        uint8_t u = 0;

        while (bis.readBits(1))
            ++u;

        _selectors[i] = tableMTF.indexToFront(u);
    }

    for (uint32_t t = 0; t < nTables; ++t)
        _tables[t].read(bis, symbolCount);

    curTbl = _selectors[0], grpIdx = 0, grpPos = 0;
}

uint32_t Tables::nextSymbol(BitInputStream &bis)
{
    if (grpPos++ % 50 == 0)
        curTbl = _selectors[grpIdx++];

    uint8_t i = _tables[curTbl].minLength();
    uint32_t codeBits = bis.readBits(i);

    for (;i <= 23; ++i)
    {
        if (codeBits <= _tables[curTbl].limit(i))
            return _tables[curTbl].symbol(codeBits - _tables[curTbl].base(i));

        codeBits = codeBits << 1 | bis.readBits(1);
    }

    return 0;
}

class Block
{
    CRC32 _crc;
    uint32_t _dec = 0, _curp = 0, *_merged;
    uint8_t _nextByte();
public:
    uint32_t process(BitInputStream &bi, uint32_t blockSize, std::ostream &os);
};

uint8_t Block::_nextByte()
{       
    uint8_t ret = _curp & 0xff;
    _curp = _merged[_curp >> 8];
    ++_dec;
    return ret;
}

uint32_t Block::process(BitInputStream &bi, uint32_t blockSize, std::ostream &os)
{
    uint32_t _blockCRC = bi.readUInt32();
    assert(bi.readBits(1) == 0);
    uint32_t bwtStartPointer = bi.readBits(24), symbolCount = 0;
    uint32_t bwtByteCounts[256] = {0};
    uint8_t symbolMap[256] = {0};

    for (uint16_t i = 0, ranges = bi.readBits(16); i < 16; ++i)
        if ((ranges & 1 << 15 >> i) != 0)
            for (uint32_t j = 0, k = i << 4; j < 16; ++j, ++k)
                if (bi.readBits(1))
                    symbolMap[symbolCount++] = uint8_t(k);

    Tables tables;
    tables.read(bi, symbolCount);
    uint8_t bwtBlock[blockSize], mtfValue = 0;
    MoveToFront symbolMTF;
    uint32_t _length = 0;

    for (uint32_t n = 0, inc = 1;;)
    {
        uint32_t nextSymbol = tables.nextSymbol(bi);

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
    uint32_t repeat = 0, acc = 0;
    int32_t _last = -1;

    while (true)
    {
        if (repeat < 1)
        {
            if (_dec == _length)
                break;

            uint8_t nextByte = _nextByte();

            if (nextByte != _last)
            {
                _last = nextByte, repeat = 1, acc = 1;
                _crc.update(nextByte);
            }
            else if (++acc == 4)
            {
                repeat = _nextByte() + 1, acc = 0;

                for (uint32_t i = 0; i < repeat; ++i)
                    _crc.update(nextByte);
            }
            else
            {
                repeat = 1;
                _crc.update(nextByte);
            }
        }

        --repeat;
        os.put(_last);
    }

    delete[] _merged;
    assert(_blockCRC == _crc.crc());
    return _crc.crc();
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

    BitInputStream bi(is);
    assert(bi.readBits(16) == 0x425a);
    bi.readBits(8);
    uint8_t blockSize = bi.readBits(8) - '0';
    uint32_t streamCRC = 0;

    while (true)
    {
        uint32_t marker1 = bi.readBits(24), marker2 = bi.readBits(24);

        if (marker1 == 0x314159 && marker2 == 0x265359)
        {
            Block b;
            uint32_t blockCRC = b.process(bi, blockSize * 100000, *os);
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

