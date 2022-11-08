#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <iostream>

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

class BitInputBase
{
public:
    virtual uint32_t readBit() = 0;
    virtual uint32_t readBits(uint32_t n) = 0;
    bool readBool();
    void ignore(int n);
    uint8_t readByte();
    uint32_t readUnary();
    void read(uint8_t *s, int n);
    void ignoreBytes(int n);
    uint32_t readUInt32();
    uint32_t readInt();
};

class Table
{
private:
    Fugt _codeLengths;
    uint16_t _pos = 0;
    uint32_t _symbolCount;
    std::array<uint32_t, 25> _bases;
    std::array<int32_t, 24> _limits;
    std::array<uint32_t, 258> _symbols;
    uint8_t _minLength(uint32_t n);
    uint8_t _maxLength(uint32_t n);
public:
    Table(uint32_t symbolCount);
    void calc();
    uint8_t maxLength();
    uint8_t minLength();
    void add(uint8_t v);
    int32_t limit(uint8_t i) const;
    uint32_t symbol(uint16_t i) const;
    uint32_t base(uint8_t i) const;
    std::string toString() const;
};

typedef std::vector<Table> Tables;

class Block
{
private:
    int32_t *_merged;
    int32_t _curTbl, _grpIdx, _grpPos, _last, _acc, _repeat, _curp, _length, _dec;
    uint32_t _nextByte();
    uint32_t _nextSymbol(BitInputBase *bi, const Tables &t, const Fugt &selectors);
public:
    void reset();
    Block() { reset(); }
    ~Block() { delete[] _merged; }
    int read();
    void init(BitInputBase *bi);
};

class BitInput : public BitInputBase
{
protected:
    uint32_t _bitBuffer = 0, _bitCount = 0;
    virtual int _getc() = 0;
public:
    uint32_t readBits(uint32_t count);
    uint32_t readBit();
};

class BitInputStream : public BitInput
{
private:
    std::istream *_is;
public:
    BitInputStream(std::istream *is);
protected:
    int _getc() override;
};


class DecStream
{
private:
    BitInputBase *_bi;
    Block _bd;
    uint32_t _streamComplete = false;
    bool _initNextBlock();
public:
    int read();
    int read(char *buf, int n);
    DecStream(BitInputBase *bi);
    void extractTo(std::ostream &os);
};



uint32_t Block::_nextSymbol(BitInputBase *bi, const Tables &t, const Fugt &selectors)
{
    if (++_grpPos % 50 == 0)
        _curTbl = selectors.at(++_grpIdx);

    Table table = t.at(_curTbl);
    uint32_t n = table.minLength();
    int32_t codeBits = bi->readBits(n);

    for (; n <= 23; n++)
    {
        if (codeBits <= table.limit(n))
            return table.symbol(codeBits - table.base(n));

        codeBits = codeBits << 1 | bi->readBits(1);
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
    for (int nextByte; _repeat < 1;)
    {           
        if (_dec == _length)
            return -1;

        if ((nextByte = _nextByte()) != _last)
            _last = nextByte, _repeat = 1, _acc = 1;
        else if (++_acc == 4)
            _repeat = _nextByte() + 1, _acc = 0;
        else
            _repeat = 1;
    }

    _repeat--;
    return _last;
}

void Block::reset()
{
    _grpIdx = _grpPos = _last = -1;
    _acc = _repeat = _length = _curp = _dec = _curTbl = 0;
}

void Block::init(BitInputBase *bi)
{
    reset();
    int32_t _bwtByteCounts[256] = {0};
    uint8_t _symbolMap[256] = {0};
    Fugt _bwtBlock2(9000000);
    bi->readInt();
    bi->readBool();
    uint32_t bwtStartPointer = bi->readBits(24), symbolCount = 0;

    for (uint16_t i = 0, ranges = bi->readBits(16); i < 16; i++)
        if ((ranges & (1 << 15 >> i)) != 0)
            for (int j = 0, k = i << 4; j < 16; j++, k++)
                if (bi->readBool())
                    _symbolMap[symbolCount++] = (uint8_t)k;

    uint32_t eob = symbolCount + 1, tables = bi->readBits(3), selectors_n = bi->readBits(15);
    MoveToFront tableMTF2;
    Fugt selectors2(selectors_n);

    for (uint32_t i = 0; i < selectors_n; i++)
    {
        uint32_t x = bi->readUnary();
        uint8_t y = tableMTF2.indexToFront(x);
        selectors2.set(i, y);
    }

    Tables tables2;

    for (uint32_t t = 0; t < tables; t++)
    {
        Table table(symbolCount);

        for (uint32_t i = 0, c = bi->readBits(5); i <= eob; i++)
        {
            while (bi->readBool()) c += bi->readBool() ? -1 : 1;
            table.add(c);
        }

        table.calc();
        tables2.push_back(table);
    }

    //cerr << tables2.toString() << "\n";
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
        }
        else if (nextSymbol == 1)
        {
            n += inc << 1;
            inc <<= 1;
        }
        else
        {
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
    }

    _merged = new int32_t[_length]();
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

uint32_t BitInputBase::readUnary()
{
    uint32_t u = 0;
    while (readBool()) u++;
    return u;
}

uint32_t BitInput::readBits(uint32_t count)
{
    if (count > 24)
        throw "Maximaal 24 bits";

    for (; _bitCount < count; _bitCount += 8)
        _bitBuffer = _bitBuffer << 8 | _getc();

    _bitCount -= count;
    return _bitBuffer >> _bitCount & ((1 << count) - 1);
}

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

uint8_t Table::_minLength(uint32_t n) { return _codeLengths.min(n); }
uint8_t Table::_maxLength(uint32_t n) { return _codeLengths.max(n); }
uint8_t Table::maxLength() { return _maxLength(_symbolCount + 2); }
uint8_t Table::minLength() { return _minLength(_symbolCount + 2); }
void Table::add(uint8_t v) { _codeLengths.set(_pos++, v); }
int32_t Table::limit(uint8_t i) const { return _limits.at(i); }
uint32_t Table::symbol(uint16_t i) const { return _symbols.at(i); }
uint32_t Table::base(uint8_t i) const { return _bases.at(i); }
uint32_t BitInput::readBit() { return readBits(1); }
BitInputStream::BitInputStream(std::istream *is) : _is(is) { }
int BitInputStream::_getc() { return _is->get(); }
bool BitInputBase::readBool() { return readBit() == 1; }
void BitInputBase::ignore(int n) { while (n--) readBool(); }
uint32_t BitInputBase::readUInt32() { return readBits(16) << 16 | readBits(16); }
uint32_t BitInputBase::readInt() { return readUInt32(); }
DecStream::DecStream(BitInputBase *bi) : _bi(bi) { _bi->ignore(32); }

int DecStream::read(char *buf, int n)
{
    int i = 0;
    for (i = 0; i < n; i++)
    {
        int c = read();

        if (c == -1)
            return i;

        buf[i] = c;
    }
    return i;
}

int DecStream::read()
{
    int nextByte = _bd.read();
    return nextByte == -1 && _initNextBlock() ? _bd.read() : nextByte;
}

bool DecStream::_initNextBlock()
{
    if (_streamComplete)
        return false;

    uint32_t marker1 = _bi->readBits(24), marker2 = _bi->readBits(24);

    if (marker1 == 0x314159 && marker2 == 0x265359)
    {
        _bd.init(_bi);
        return true;
    }

    if (marker1 == 0x177245 && marker2 == 0x385090)
    {
        _streamComplete = true;
        _bi->readInt();
        return false;
    }

    _streamComplete = true;
    throw "BZip2 stream format error";
}

int main(int argc, char **argv)
{
    BitInputStream bi(&std::cin);
    DecStream ds(&bi);
    for (int b; (b = ds.read()) != -1; std::cout.put(b));
    return 0;
}

