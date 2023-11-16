#include <cstdint>
#include <algorithm>
#include <numeric>
#include <cstdio>

class BitInputStream
{
    FILE *_is;
    uint32_t _bitBuffer = 0, _bitCount = 0;
public:
    BitInputStream(FILE *is) : _is(is) { }
    bool readBool() { return readBits(1) == 1; }
    uint32_t readUInt32() { return readBits(16) << 16 | readBits(16); }
    uint32_t readUnary() { uint32_t u = 0; while (readBool()) ++u; return u; }

    uint32_t readBits(uint32_t count)
    {
        if (count > 24)
            throw "Maximaal 24 bits";

        for (; _bitCount < count; _bitCount += 8)
            _bitBuffer = _bitBuffer << 8 | fgetc(_is);

        _bitCount -= count;
        return _bitBuffer >> _bitCount & (1 << count) - 1;
    }
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

static uint32_t process_block(BitInputStream &bi, uint32_t blockSize, FILE *os)
{
    uint32_t bwtByteCounts[256] = {0};
    uint8_t symbolMap[256] = {0};
    uint32_t _blockCRC = bi.readUInt32();

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
    uint8_t tableMTF[256];
    uint8_t symbolMTF[256];
    std::iota(tableMTF, tableMTF + 256, 0);
    std::iota(symbolMTF, symbolMTF + 256, 0);
    uint32_t _length = 0;

    for (uint32_t i = 0; i < nSelectors; ++i)
    {
        uint8_t foo = bi.readUnary();
        std::rotate(tableMTF, tableMTF + foo, tableMTF + foo + 1);
        selectors[i] = tableMTF[0];
    }

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

        std::rotate(symbolMTF, symbolMTF + nextSymbol - 1, symbolMTF + nextSymbol);
        mtfValue = symbolMTF[0];
        uint8_t nextByte = symbolMap[mtfValue];
        bwtByteCounts[nextByte]++;
        bwtBlock[_length++] = nextByte;
    }

    uint32_t *_merged = new uint32_t[_length];
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

    uint32_t _curp = _merged[bwtStartPointer], repeat = 0, acc = 0, _dec = 0;
    int32_t _last = -1;
    CRC32 _crc;

    while (true)
    {
        if (repeat < 1)
        {
            if (_dec == _length)
                break;

            uint8_t nextByte = _curp & 0xff;
            _curp = _merged[_curp >> 8];
            ++_dec;

            if (nextByte != _last)
            {
                _last = nextByte, repeat = 1, acc = 1;
                _crc.update(nextByte);
            }
            else if (++acc == 4)
            {
                repeat = (_curp & 0xff) + 1, acc = 0;
                _curp = _merged[_curp >> 8], ++_dec;

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
        fputc(_last, os);
    }

    delete[] _merged;

    if (_blockCRC != _crc.crc())
        throw "Block CRC mismatch";

    return _crc.crc();
}

int main(int argc, char **argv)
{
    FILE *os = stdout, *msg = stderr, *is = stdin;

    if (argc == 2)
        is = fopen(argv[1], "r");

    BitInputStream bi(is);
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
            uint32_t blockCRC = process_block(bi, blockSize * 100000, os);
            streamCRC = (streamCRC << 1 | streamCRC >> 31) ^ blockCRC;
            continue;
        }

        if (marker1 == 0x177245 && marker2 == 0x385090)
        {
            uint32_t crc = bi.readUInt32();
            fprintf(msg, "0x%08x 0x%08x\r\n", crc, streamCRC);
            break;
        }

        throw "format error!";
    }

    fclose(is);
    return 0;
}

