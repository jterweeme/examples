#include <fstream>
#include <iostream>
#include <cstdint>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

class BitInputStream
{
private:
    std::istream *_is;
    int _getc() { return _is->get(); }
    uint32_t _bitBuffer = 0, _bitCount = 0;
public:
    BitInputStream(std::istream *is) : _is(is) { }
    uint32_t readBits(uint32_t count);
    uint32_t readBit() { return readBits(1); }
    bool readBool() { return readBit() == 1; }
    void ignore(int n) { while (n--) readBool(); }
    uint8_t readByte();
    uint32_t readUnary();
    void read(uint8_t *s, int n);
    void ignoreBytes(int n);
    uint32_t readUInt32() { return readBits(16) << 16 | readBits(16); }
    uint32_t readInt() { return readUInt32(); }
};

uint32_t BitInputStream::readUnary()
{
    uint32_t u = 0;
    while (readBool()) u++;
    return u;
}

uint32_t BitInputStream::readBits(uint32_t count)
{
    if (count > 24)
        throw "Maximaal 24 bits";

    for (; _bitCount < count; _bitCount += 8)
        _bitBuffer = _bitBuffer << 8 | _getc();

    _bitCount -= count;
    return _bitBuffer >> _bitCount & ((1 << count) - 1);
}

class MoveToFront
{
private:
    uint8_t _buf[256];
public:
    MoveToFront() { for (uint32_t i = 0; i < 256; ++i) _buf[i] = i; }
    uint8_t indexToFront(uint32_t index);
};

uint8_t MoveToFront::indexToFront(uint32_t index)
{
    uint8_t value = _buf[index];

    for (uint32_t i = index; i > 0; i--)
        _buf[i] = _buf[i - 1];

    return _buf[0] = value;
}

class Table
{
private:
    uint8_t _codeLengths[258];
    uint16_t _pos = 0;
    uint32_t _symbolCount;
    uint32_t _bases[25] = {0};
    int32_t _limits[24] = {0};
    uint32_t _symbols[258] = {0};
public:
    void symbolCount(uint32_t n) { _symbolCount = n; }
    void calc();
    uint8_t minLength()
    {
        uint8_t ret = 23;
        for (uint32_t i = 0; i < _symbolCount + 2; ++i)
            ret = std::min(_codeLengths[i], ret);
        return ret;
    }

    void add(uint8_t v) { _codeLengths[_pos++] = v; }
    int32_t limit(uint8_t i) const { return _limits[i]; }
    uint32_t symbol(uint16_t i) const { return _symbols[i]; }
    uint32_t base(uint8_t i) const { return _bases[i]; }
};

void Table::calc()
{
    for (uint32_t i = 0; i < _symbolCount + 2; ++i)
        _bases[_codeLengths[i] + 1]++;

    for (uint32_t i = 1; i < 25; ++i)
        _bases[i] += _bases[i - 1];

    uint8_t minLength2 = 23;
    uint8_t maxLength2 = 0;

    for (uint32_t i = 0; i < _symbolCount + 2; ++i)
    {
        minLength2 = std::min(_codeLengths[i], minLength2);
        maxLength2 = std::max(_codeLengths[i], maxLength2);
    }

    for (int32_t i = minLength2, code = 0; i <= maxLength2; ++i)
    {
        int32_t base = code;
        code += _bases[i + 1] - _bases[i];
        _bases[i] = base - _bases[i];
        _limits[i] = code - 1;
        code <<= 1;
    }

    for (uint32_t i = 0; minLength2 <= maxLength2; ++minLength2)
        for (uint32_t symbol = 0; symbol < _symbolCount + 2; ++symbol)
            if (_codeLengths[symbol] == minLength2)
                _symbols[i++] = symbol;
}

class CRC32
{
private:
    uint32_t _table[256];
    uint32_t _crc = 0xffffffff;
public:
    CRC32();
    void reset() { _crc = 0xffffffff; }
    void update(int c) { _crc = (_crc << 8) ^ _table[((_crc >> 24) ^ c) & 0xff]; }
    uint32_t crc() const { return ~_crc; }
};

CRC32::CRC32()
{
    //https://github.com/gcc-mirror/gcc/blob/master/libiberty/crc32.c
    for (uint32_t i = 0, j, c; i < 256; ++i)
    {
        for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : c << 1;
        _table[i] = c;
    }
}

class Block
{
private:
    CRC32 _crc;
    uint32_t _blockCRC;
    int32_t *_merged = nullptr;
    int32_t _last, _acc, _curp, _length, _dec;
    uint32_t _repeat;
    uint32_t _nextByte();
    int inline _read();
public:
    ~Block() { delete[] _merged; }
    void write(std::ostream &os);
    void init(BitInputStream &bi, size_t blockSize);
    uint32_t checkCRC() const;
};

uint32_t Block::checkCRC() const
{
    if (_blockCRC != _crc.crc())
        throw "Block CRC mismatch";

    return _crc.crc();
}

uint32_t Block::_nextByte()
{       
    int r = _curp & 0xff;
    _curp = _merged[_curp >> 8];
    ++_dec;
    return r;
}

void Block::write(std::ostream &os)
{
    _repeat = _dec = 0;

    for (int c; (c = _read()) != -1;)
        os.put(c);
}

int Block::_read()
{       
    while (_repeat < 1)
    {           
        if (_dec == _length)
            return -1;

        int nextByte = _nextByte();

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

    _repeat--;
    return _last;
}

void Block::init(BitInputStream &bi, size_t blockSize)
{
    _crc.reset();
    _last = -1;
    _acc = _length = _curp = 0;
    int32_t _bwtByteCounts[256] = {0};
    uint8_t _symbolMap[256] = {0};
    _blockCRC = bi.readInt();
    bool randomised = bi.readBool();

    if (randomised)
        throw "Randomised blocks not supported.";

    uint32_t bwtStartPointer = bi.readBits(24), symbolCount = 0;

    for (uint16_t i = 0, ranges = bi.readBits(16); i < 16; ++i)
        if ((ranges & 1 << 15 >> i) != 0)
            for (int j = 0, k = i << 4; j < 16; ++j, ++k)
                if (bi.readBool())
                    _symbolMap[symbolCount++] = uint8_t(k);

    uint32_t eob = symbolCount + 1, nTables = bi.readBits(3), selectors_n = bi.readBits(15);
    MoveToFront tableMTF2;
    uint8_t selectors2[selectors_n];

    for (uint32_t i = 0; i < selectors_n; ++i)
        selectors2[i] = tableMTF2.indexToFront(bi.readUnary());

    Table *tables2[nTables];
    uint8_t _bwtBlock2[blockSize];

    //read the canonical Huffman code lengths for each table
    for (uint32_t t = 0; t < nTables; ++t)
    {
        Table *table = new Table();
        table->symbolCount(symbolCount);

        for (uint32_t i = 0, c = bi.readBits(5); i <= eob; ++i)
        {
            while (bi.readBool())
                c += bi.readBool() ? -1 : 1;
            table->add(c);
        }

        table->calc();
        tables2[t] = table;
    }

    int32_t curTbl = selectors2[0];
    MoveToFront symbolMTF2;
    _length = 0;
    uint32_t grpIdx = 0, grpPos = 0, inc = 1;
    uint8_t mtfValue = 0;

    for (int n = 0;;)
    {
        uint32_t nextSymbol = 0;
        
        {
            if (grpPos++ % 50 == 0)
                curTbl = selectors2[grpIdx++];
    
            Table *table = tables2[curTbl];
            uint32_t i = table->minLength();
            int32_t codeBits = bi.readBits(i);
    
            while (i <= 23)
            {
                if (codeBits <= table->limit(i))
                {
                    nextSymbol = table->symbol(codeBits - table->base(i));
                    break;
                }
    
                codeBits = codeBits << 1 | bi.readBits(1);
                ++i;
            }
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
            uint8_t nextByte = _symbolMap[mtfValue];
            _bwtByteCounts[nextByte] += n;

            while (--n >= 0)
                _bwtBlock2[_length++] = nextByte;

            n = 0, inc = 1;
        }

        if (nextSymbol == eob)
            break;

        mtfValue = symbolMTF2.indexToFront(nextSymbol - 1);
        uint8_t nextByte = _symbolMap[mtfValue];
        _bwtByteCounts[nextByte]++;
        _bwtBlock2[_length++] = nextByte;
    }

    for (Table *t : tables2)
        delete t;

    if (_merged)
        delete[] _merged;
    _merged = new int32_t[_length];
    int characterBase[256] = {0};

    for (uint16_t i = 0; i < 255; ++i)
        characterBase[i + 1] = _bwtByteCounts[i];

    for (uint16_t i = 2; i <= 255; ++i)
        characterBase[i] += characterBase[i - 1];

    for (int32_t i = 0; i < _length; ++i)
    {
        int value = _bwtBlock2[i] & 0xff;
        _merged[characterBase[value]++] = (i << 8) + value;
    }

    _curp = _merged[bwtStartPointer];
}

class Toolbox
{
public:
    static char nibble(uint8_t n) { return n <= 9 ? '0' + char(n) : 'a' + char(n - 10); }
    static std::string hex32(uint32_t dw);
};

std::string Toolbox::hex32(uint32_t dw)
{
    std::string ret;
    for (uint32_t i = 0; i <= 28; i += 4)
        ret.push_back(nibble(dw >> (28 - i) & 0xf));
    return ret;
}

static void decompress(std::istream &is, std::ostream &os)
{
    BitInputStream bi(&is);
    uint16_t magic = bi.readBits(16);

    if (magic != 0x425a)
        throw "invalid magic";

    uint8_t foo = bi.readBits(8);
    uint8_t blockSize = bi.readBits(8) - '0';
    Block bd;
    uint32_t streamCRC = 0;

    while (true)
    {
        uint32_t marker1 = bi.readBits(24), marker2 = bi.readBits(24);

        if (marker1 == 0x314159 && marker2 == 0x265359)
        {
            bd.init(bi, blockSize * 100000);
            bd.write(os);
            uint32_t blockCRC = bd.checkCRC();
            streamCRC = (streamCRC << 1 | streamCRC >> 31) ^ blockCRC;
            continue;
        }

        if (marker1 == 0x177245 && marker2 == 0x385090)
        {
            uint32_t crc = bi.readInt();
            Toolbox t;
            std::cerr << "0x" << t.hex32(crc) << " 0x" << t.hex32(streamCRC) << "\r\n";
            break;
        }

        throw "format error!";
    }
}

int main(int argc, char **argv)
{
    std::istream *is = &std::cin;
    std::ifstream ifs;

    if (argc == 2)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

    decompress(*is, std::cout);
    ifs.close();
    return 0;
}

