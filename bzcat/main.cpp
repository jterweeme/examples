#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstdint>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

template<class T> class Fector 
{
private:
    uint32_t _size;
    uint32_t _pos = 0;
    T *_buf;
    T _max(T a, T b) { return a > b ? a : b; }
    T _min(T a, T b) { return a < b ? a : b; }
public:
    typedef T *iterator;
    typedef T *const_iterator; 
    
    Fector& operator= (const Fector &f)
    {
        _pos = f._pos;
        _size = f._size;
        _buf = new T[_size];

        for (uint32_t i = 0; i < _size; i++)
            _buf[i] = f._buf[i];
    
        return *this;
    }

    Fector(uint32_t size) : _size(size), _buf(new T[size]) { }

    Fector(const Fector &f) : _size(f._size), _buf(new T[_size])
    { for (uint32_t i = 0; i < _size; i++) _buf[i] = f._buf[i]; }
    
    ~Fector() { delete[] _buf; }
    uint32_t size() const { return _size; }
    T at(uint32_t i) const { return _buf[i]; }
    T set(uint32_t i, T val) { return _buf[i] = val; }

    T max(uint32_t range)
    {
        T a = 0;

        for (uint32_t i = 0; i < range; i++)
            a = _max(_buf[i], a);

        return a;
    }

    T min(uint32_t range)
    {
        T a = 0;
        for (uint32_t i = 0; i < range; i++) a = _min(_buf[i], a);
        return a;
    }

    bool isFull() const { return _pos >= _size; }
    void testFull() const { if (isFull()) throw "Fector is full"; }
    T max() { return max(_size); }
    T min() { return min(_size); }
    T *begin() const { return _buf; }
    T *end() const { return _buf + _size; }
    void push_back(const T &x) { testFull(); _buf[_pos++] = x; }
    T &operator[](uint32_t i) { return _buf[i]; }
};

typedef Fector<uint8_t> Fugt;

class MoveToFront : public Fugt
{
public:
    MoveToFront();
    uint8_t indexToFront(uint32_t index);
};

MoveToFront::MoveToFront() : Fugt(256)
{
    for (uint32_t i = 0; i < 256; i++)
        set(i, i);
}

uint8_t MoveToFront::indexToFront(uint32_t index)
{
    uint8_t value = at(index);

    for (uint32_t i = index; i > 0; i--)
        set(i, at(i - 1));

    return set(0, value);
}

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

class Table
{
private:
    Fugt _codeLengths;
    uint16_t _pos = 0;
    uint32_t _symbolCount;
    std::array<uint32_t, 25> _bases;
    std::array<int32_t, 24> _limits;
    std::array<uint32_t, 258> _symbols;
    uint8_t _minLength(uint32_t n) { return _codeLengths.min(n); }
    uint8_t _maxLength(uint32_t n) { return _codeLengths.max(n); }
public:
    Table(uint32_t symbolCount);
    void calc();
    uint8_t maxLength() { return _maxLength(_symbolCount + 2); }
    uint8_t minLength() { return _minLength(_symbolCount + 2); }
    void add(uint8_t v) { _codeLengths.set(_pos++, v); }
    int32_t limit(uint8_t i) const { return _limits.at(i); }
    uint32_t symbol(uint16_t i) const { return _symbols.at(i); }
    uint32_t base(uint8_t i) const { return _bases.at(i); }
};

Table::Table(uint32_t symbolCount) : _codeLengths(258), _symbolCount(symbolCount)
{
    _bases.fill(0);
    _limits.fill(0);
    _symbols.fill(0);
}   

void Table::calc()
{
    for (uint32_t i = 0; i < _symbolCount + 2; i++)
        _bases[_codeLengths.at(i) + 1]++;

    for (uint32_t i = 1; i < 25; i++)
        _bases.at(i) += _bases.at(i - 1);

    uint8_t minLength2 = minLength();
    uint8_t maxLength2 = maxLength();

    for (int32_t i = minLength2, code = 0; i <= maxLength2; i++)
    {
        int32_t base = code;
        code += _bases.at(i + 1) - _bases.at(i);
        _bases.at(i) = base - _bases.at(i);
        _limits.at(i) = code - 1;
        code <<= 1;
    }

    uint8_t n = minLength2;

    for (uint32_t i = 0; n <= maxLength2; n++)
        for (uint32_t symbol = 0; symbol < _symbolCount + 2; symbol++)
            if (_codeLengths.at(symbol) == n)
                _symbols.at(i++) = symbol;
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
#if 0
    for (uint32_t x : _table)
        std::cerr << "0x" << hex32(x) << "\r\n";
#endif
}

class Block
{
private:
    CRC32 _crc;
    int32_t *_merged = nullptr;
    int32_t _curTbl, _grpIdx, _grpPos, _last, _acc, _repeat, _curp, _length, _dec;
    uint32_t _nextByte();
    uint32_t _nextSymbol(BitInputStream &bi, const std::vector<Table> &t, const Fugt &selectors);
public:
    void reset();
    Block() { reset(); }
    ~Block() { delete[] _merged; }
    int read();
    void init(BitInputStream &bi);
};

uint32_t Block::_nextSymbol(BitInputStream &bi, const std::vector<Table> &t, const Fugt &selectors)
{
    if (++_grpPos % 50 == 0)
        _curTbl = selectors.at(++_grpIdx);

    Table table = t.at(_curTbl);
    uint32_t n = table.minLength();
    int32_t codeBits = bi.readBits(n);

    for (; n <= 23; n++)
    {
        if (codeBits <= table.limit(n))
            return table.symbol(codeBits - table.base(n));

        codeBits = codeBits << 1 | bi.readBits(1);
    }   
    
    return 0;
}           
                
uint32_t Block::_nextByte()
{       
    int r = _curp & 0xff;
    _curp = _merged[_curp >> 8];
    _dec++;
    return r;   
}   

int Block::read()
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

            for (int i = 0; i < _repeat; ++i)
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

void Block::reset()
{
    _crc.reset();
    _grpIdx = _grpPos = _last = -1;
    _acc = _repeat = _length = _curp = _dec = _curTbl = 0;
}

void Block::init(BitInputStream &bi)
{
    reset();
    int32_t _bwtByteCounts[256] = {0};
    uint8_t _symbolMap[256] = {0};
    Fugt _bwtBlock2(9000000);
    uint32_t blockCRC = bi.readInt();
    bool randomised = bi.readBool();

    if (randomised)
        throw "Randomised blocks not supported.";

    uint32_t bwtStartPointer = bi.readBits(24), symbolCount = 0;

    for (uint16_t i = 0, ranges = bi.readBits(16); i < 16; i++)
        if ((ranges & (1 << 15 >> i)) != 0)
            for (int j = 0, k = i << 4; j < 16; j++, k++)
                if (bi.readBool())
                    _symbolMap[symbolCount++] = (uint8_t)k;

    uint32_t eob = symbolCount + 1, tables = bi.readBits(3), selectors_n = bi.readBits(15);
    MoveToFront tableMTF2;
    Fugt selectors2(selectors_n);

    for (uint32_t i = 0; i < selectors_n; i++)
    {
        uint32_t x = bi.readUnary();
        uint8_t y = tableMTF2.indexToFront(x);
        selectors2.set(i, y);
    }

    std::vector<Table> tables2;

    for (uint32_t t = 0; t < tables; t++)
    {
        Table table(symbolCount);

        for (uint32_t i = 0, c = bi.readBits(5); i <= eob; i++)
        {
            while (bi.readBool()) c += bi.readBool() ? -1 : 1;
            table.add(c);
        }

        table.calc();
        tables2.push_back(table);
    }

    _curTbl = selectors2.at(0);
    MoveToFront symbolMTF2;
    _length = 0;

    for (int n = 0, inc = 1, mtfValue = 0;;)
    {
        uint32_t nextSymbol = _nextSymbol(bi, tables2, selectors2);

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
                _bwtBlock2.set(_length++, nextByte);

            n = 0;
            inc = 1;
        }

        if (nextSymbol == eob)
            break;

        mtfValue = symbolMTF2.indexToFront(nextSymbol - 1);
        uint8_t nextByte = _symbolMap[mtfValue];
        _bwtByteCounts[nextByte]++;
        _bwtBlock2.set(_length++, nextByte);
    }

    if (_merged)
        delete[] _merged;
    _merged = new int32_t[_length];
    int characterBase[256] = {0};

    for (int i = 0; i < 255; i++)
        characterBase[i + 1] = _bwtByteCounts[i];

    for (int i = 2; i <= 255; i++)
        characterBase[i] += characterBase[i - 1];

    for (int32_t i = 0; i < _length; i++)
    {
        int value = _bwtBlock2.at(i) & 0xff;
        _merged[characterBase[value]++] = (i << 8) + value;
    }

    _curp = _merged[bwtStartPointer];
}

static char nibble(uint8_t n)
{
    return n <= 9 ? '0' + char(n) : 'a' + char(n - 10);
}

static std::string hex32(uint32_t dw)
{
    std::string ret;
    ret.push_back(nibble(dw >> 28 & 0xf));
    ret.push_back(nibble(dw >> 24 & 0xf));
    ret.push_back(nibble(dw >> 20 & 0xf));
    ret.push_back(nibble(dw >> 16 & 0xf));
    ret.push_back(nibble(dw >> 12 & 0xf));
    ret.push_back(nibble(dw >>  8 & 0xf));
    ret.push_back(nibble(dw >>  4 & 0xf));
    ret.push_back(nibble(dw >>  0 & 0xf));
    return ret;
}

static void decompress(std::istream &is, std::ostream &os)
{
    BitInputStream bi(&is);
    bi.ignore(32);
    Block bd;

    while (true)
    {
        uint32_t marker1 = bi.readBits(24), marker2 = bi.readBits(24);

        if (marker1 == 0x314159 && marker2 == 0x265359)
        {
            bd.init(bi);

            for (int c; (c = bd.read()) != -1;)
                os.put(c);

            continue;
        }

        if (marker1 == 0x177245 && marker2 == 0x385090)
        {
            uint32_t crc = bi.readInt();
            std::cerr << "0x" << hex32(crc) << "\r\n";
            return;
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

