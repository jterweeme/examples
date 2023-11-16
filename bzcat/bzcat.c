#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

FILE *os, *msg, *is;
uint32_t _bitBuffer = 0, _bitCount = 0;
uint32_t g_crc_table[256];

static uint32_t readBits(uint32_t count)
{   assert(count <= 24);
    for (; _bitCount < count; _bitCount += 8)
        _bitBuffer = _bitBuffer << 8 | fgetc(is);
    _bitCount -= count;
    return _bitBuffer >> _bitCount & (1 << count) - 1;
}

static uint32_t readUInt32() { return readBits(16) << 16 | readBits(16); }

void crc_update(uint32_t *crc, uint8_t c)
{ *crc = *crc << 8 ^ g_crc_table[(*crc >> 24 ^ c) & 0xff]; }

static uint32_t process_block(uint32_t blockSize, FILE *os)
{
    uint32_t _blockCRC = readUInt32();

    //randomised blocks not supported
    assert(readBits(1) == 0);

    uint32_t bwtStartPointer = readBits(24), symbolCount = 0;
    uint8_t symbolMap[256] = {0};

    for (uint16_t i = 0, ranges = readBits(16); i < 16; ++i)
        if ((ranges & 1 << 15 >> i) != 0)
            for (uint32_t j = 0, k = i << 4; j < 16; ++j, ++k)
                if (readBits(1))
                    symbolMap[symbolCount++] = (uint8_t)(k);

    uint8_t nTables = readBits(3);
    uint16_t nSelectors = readBits(15);
    uint8_t bwtBlock[blockSize], selectors[nSelectors];
    uint8_t tableMTF[256], symbolMTF[256];
    uint32_t _length = 0;

    for (uint16_t i = 0; i < 256; ++i)
        tableMTF[i] = symbolMTF[i] = i;

    for (uint32_t i = 0; i < nSelectors; ++i)
    {
        uint16_t idx = 0;

        while (readBits(1))
            ++idx;

        uint8_t value = tableMTF[idx];

        for (uint16_t i = idx; i > 0; i--)
            tableMTF[i] = tableMTF[i - 1];

        selectors[i] = tableMTF[0] = value;
    }
    uint32_t _bases[6][25] = {0}, _limits[6][24] = {0}, _symbols[6][258] = {0};
    uint8_t _minLen[6] = {23,23,23,23,23,23}, _maxLen[6] = {0}, mtfValue = 0;
    uint32_t bwtByteCounts[256] = {0};

    for (uint8_t t = 0, codeLengths[258] = {0}; t < nTables; ++t)
    {
        for (uint32_t i = 0, pos = 0, c = readBits(5); i <= symbolCount + 1; ++i)
        {
            while (readBits(1))
                c += readBits(1) ? -1 : 1;
            codeLengths[pos++] = c;
        }

        for (uint32_t i = 0; i < symbolCount + 2; ++i)
            _bases[t][codeLengths[i] + 1]++;

        for (uint32_t i = 1; i < 25; ++i)
            _bases[t][i] += _bases[t][i - 1];

        for (uint32_t i = 0; i < symbolCount + 2; ++i)
        {
            _minLen[t] = codeLengths[i] < _minLen[t] ? codeLengths[i] : _minLen[t];
            _maxLen[t] = codeLengths[i] > _maxLen[t] ? codeLengths[i] : _maxLen[t];
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

    for (uint32_t n = 0, grpIdx = 0, grpPos = 0, inc = 1, curTbl = selectors[0]; true;)
    {
        if (grpPos++ % 50 == 0)
            curTbl = selectors[grpIdx++];

        uint8_t i = _minLen[curTbl];
        uint32_t codeBits = readBits(i);
        uint32_t nextSymbol = 0;

        for (;i <= 23; ++i)
        {
            if (codeBits <= _limits[curTbl][i])
            {
                nextSymbol = _symbols[curTbl][codeBits - _bases[curTbl][i]];
                break;
            }
            codeBits = codeBits << 1 | readBits(1);
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
            while (--n >= 1) bwtBlock[_length++] = nextByte;
        }
        //end of block
        if (nextSymbol == symbolCount + 1) break;

        mtfValue = symbolMTF[nextSymbol - 1];

        for (uint16_t i = nextSymbol - 1; i > 0; --i)
            symbolMTF[i] = symbolMTF[i - 1];

        symbolMTF[0] = mtfValue;
        uint8_t nextByte = symbolMap[mtfValue];
        bwtByteCounts[nextByte]++;
        bwtBlock[_length++] = nextByte;
    }
    uint32_t *_merged = malloc(_length * sizeof(uint32_t));
    uint32_t characterBase[256] = {0};
    int32_t _last = -1;
    uint32_t _crc = 0xffffffff;

    for (uint16_t i = 0; i < 255; ++i)
        characterBase[i + 1] = bwtByteCounts[i];

    for (uint16_t i = 2; i <= 255; ++i)
        characterBase[i] += characterBase[i - 1];

    for (uint32_t i = 0; i < _length; ++i)
    {
        uint8_t value = bwtBlock[i] & 0xff;
        _merged[characterBase[value]++] = (i << 8) + value;
    }

    for (uint32_t _curp = _merged[bwtStartPointer], repeat = 0, acc = 0, _dec = 0; true; )
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
                crc_update(&_crc, nextByte);
            }
            else if (++acc == 4)
            {
                repeat = (_curp & 0xff) + 1, acc = 0;
                _curp = _merged[_curp >> 8], ++_dec;

                for (uint32_t i = 0; i < repeat; ++i)
                    crc_update(&_crc, nextByte);
            }
            else
            {
                repeat = 1;
                crc_update(&_crc, nextByte);
            }
        }

        --repeat;
        fputc(_last, os);
    }

    free(_merged);
    assert(_blockCRC == ~_crc);
    return ~_crc;
}

int main(int argc, char **argv)
{
    is = stdin, os = stdout, msg = stderr;
    if (argc == 2)
        is = fopen(argv[1], "r");
    uint16_t magic = readBits(16);
    assert(magic == 0x425a);
    readBits(8);
    uint8_t blockSize = readBits(8) - '0';
    uint32_t streamCRC = 0;

    //init global crc table
    for (uint32_t i = 0, j, c; i < 256; ++i)
    {   for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? c << 1 ^ 0x04c11db7 : c << 1;
        g_crc_table[i] = c;
    }

    while (true)
    {
        uint32_t marker1 = readBits(24), marker2 = readBits(24);

        if (marker1 == 0x314159 && marker2 == 0x265359)
        {
            uint32_t blockCRC = process_block(blockSize * 100000, os);
            streamCRC = (streamCRC << 1 | streamCRC >> 31) ^ blockCRC;
            continue;
        }

        if (marker1 == 0x177245 && marker2 == 0x385090)
        {
            uint32_t crc = readUInt32();
            fprintf(msg, "0x%08x 0x%08x\r\n", crc, streamCRC);
            break;
        }

        //throw "format error!";
    }

    fclose(is);
    return 0;
}

