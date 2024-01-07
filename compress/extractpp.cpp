//This is a comment
//I love comments

#include <cassert>
#include <cstdint>
#include <exception>
#include <coroutine>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

using std::exception_ptr;
using std::convertible_to;
using std::suspend_always;
using std::coroutine_handle;
using std::current_exception;
using std::move;
using std::rethrow_exception;
using std::runtime_error;
using std::forward; 

//boilerplate
template <typename T> class Generator
{
public:
    struct promise_type
    {
        T value;
        exception_ptr exception_;
        suspend_always initial_suspend() { return {}; }
        suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception_ = current_exception(); }
        void return_void() {}

        Generator get_return_object()
        { return Generator(coroutine_handle<promise_type>::from_promise(*this)); }
                                                                             
        template <convertible_to<T> From> suspend_always yield_value(From &&from)
        {
            value = forward<From>(from);
            return {};
        }
    };
private:
    coroutine_handle<promise_type> _h;
    bool _full = false;

    void _fill()
    {
        if (_full)
            return;

        _h();
    
        if (_h.promise().exception_)
            rethrow_exception(_h.promise().exception_);
 
        _full = true;
    }
public:
    Generator(coroutine_handle<promise_type> h) : _h(h) {}
    ~Generator() { _h.destroy(); }
    explicit operator bool() { _fill(); return !_h.done(); }

    T operator()()
    {
        _fill();
        _full = false; 
        return move(_h.promise().value);
    }
};

namespace fast
{
class istream
{
private:
    uint32_t _cap;
    uint32_t _head = 0, _tail = 0;
    uint8_t *_buf;
protected:
    int _fd;
public:
    ~istream() { delete[] _buf; }
    istream &read(char *s, size_t n) { assert(false); }
    size_t gcount() const { assert(false); return 0; }

    istream(int fd = -1, uint32_t capacity = 8192)
      : _cap(capacity), _buf(new uint8_t[capacity]), _fd(fd) { }

    int get()
    {
        if (_tail == _head)
        {
            ssize_t n = ::read(_fd, _buf, _cap);
            if (n < 1) return -1;
            _head = n;
            _tail = 0;
        }
        return _buf[_tail++];
    }
};

class ifstream : public istream
{
public:
    void close() { ::close(_fd); }
    void open(const char *fn) { _fd = ::open(fn, O_RDONLY); }
};

class ostream
{
    int _fd;
    uint32_t _cap;
    uint32_t _pos = 0;
    char *_buf;
public:
    ostream(int fd, uint32_t capacity) : _fd(fd), _cap(capacity), _buf(new char[capacity]) { }
    ~ostream() { delete[] _buf; }
    inline void put(char c) { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }
    inline ostream& operator<<(const char *s) { while (*s) put(*s++); return *this; }

    //LZW is sneller dan int naar string!
    inline ostream& operator<<(unsigned n)
    {
        if (n == 0) { put('0'); return *this; }
        char buf[100];
        unsigned i = 0;
        for (; n; n = n / 10)
            buf[i++] = n % 10 + '0';
        while (i)
            put(buf[--i]);
        return *this;
    }
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(2, 8192);
}

#if 0
using fast::istream;
using fast::ostream;
using fast::ifstream;
using fast::cin;
using fast::cout;
using fast::cerr;
#else
using std::istream;
using std::ostream;
using std::ifstream;
using std::cin;
using std::cout;
using std::cerr;
#endif

class Toolbox
{
public:
    template <class T> static T min(T a, T b) { return b < a ? b : a; }
    template <class T> static T max(T a, T b) { return a < b ? b : a; }

    static char nibble(uint8_t n)
    { return n <= 9 ? '0' + char(n) : 'a' + char(n - 10); }

    static void hex32(ostream &os, unsigned dw, unsigned nibbles = 8)
    { for (unsigned i = 32 - nibbles * 4; i != 32; i += 4) os.put(nibble(dw >> 28 - i & 0xf)); }
};

class BitStream
{
    istream &_is;
    unsigned _bits = 0, _window = 0, _cnt = 0;
public:
    BitStream(istream &is) : _is(is) { }
    unsigned cnt() const { return _cnt; }

    int readBits(unsigned n)
    {
        for (; _bits < n; _bits += 8)
        {
            int c = _is.get();
            if (c == -1) return -1;
            _window |= c << _bits;
        }

        int ret = _window & (1 << n) - 1;
        _window >>= n, _bits -= n, _cnt += n;
        return ret;
    }
};

class Codes1
{
    BitStream _bis;
    const unsigned _maxbits;
    unsigned _cnt = 0, _nbits = 9;
public:
    Codes1(istream &is, unsigned maxbits) : _bis(is), _maxbits(maxbits)
    {
        assert(maxbits >= 9 && maxbits <= 16);
    }

    int extract()
    {
        int code = _bis.readBits(_nbits);

        //read 256 9-bit codes, 512 10-bit codes, 1024 11-bit codes, etc
        if (++_cnt == 1U << _nbits - 1U && _nbits != _maxbits)
            ++_nbits, _cnt = 0;

        //and x number of maxbit (usually 16-bit) codes until code 256
        if (code == 256)
        {
            //padding (blocks of 8 codes)
            while (_cnt++ % 8)
                _bis.readBits(_nbits);

            _cnt = 0, _nbits = 9;
        }

        return code;
    }
};

static Generator<unsigned> codes1(istream &is, unsigned bitdepth)
{
    BitStream bis(is);
    unsigned cnt = 0, nbits = 9;

    for (int code; (code = bis.readBits(nbits)) != -1;)
    {
        if (++cnt == 1U << nbits - 1 && nbits != bitdepth)
            ++nbits, cnt = 0;

        if (code == 256)
        {
            while (cnt++ % 8)
                bis.readBits(nbits);

            cnt = 0, nbits = 9;
        }

        co_yield code;
    }
}

class Codes2
{
    char _buf[20];
    unsigned _nbufcodes = 0;
    istream &_is;
    const unsigned _maxbits;
    unsigned _nbits = 9;
    unsigned _bits = 0;
    unsigned _i = 0;
    unsigned _ncodesblk = 0;
public:
    Codes2(istream &is, unsigned maxbits) : _is(is), _maxbits(maxbits)
    {
        assert(maxbits >= 9 && maxbits <= 16);
    }

    int extract()
    {
        if (_nbufcodes == 0)
        {
            _is.read(_buf, _nbits);
            _nbufcodes = _is.gcount() * 8 / _nbits;
            _i = 0;
            _bits = 0;

            if (_nbufcodes <= 0)
                return -1;
        }

        --_nbufcodes;
        unsigned shift = _i++ * (_nbits - 8) % 8;
        unsigned *window = (unsigned *)(_buf + _bits / 8);
        unsigned code = *window >> shift & (1 << _nbits) - 1;
        _bits += _nbits;
        ++_ncodesblk;

        if (code == 256)
            _nbits = 9, _ncodesblk = 0, _nbufcodes = 0;
        else if (_ncodesblk == 1U << _nbits - 1U && _nbits != _maxbits)
            ++_nbits, _ncodesblk = 0;

        return code;
    }
};

static Generator<unsigned> codes2(istream &is, unsigned maxbits)
{
    char buf[20];
    
    for (unsigned ncodes = 0, nbits = 9; true;)
    {
        is.read(buf, nbits);
        unsigned nbufcodes = is.gcount() * 8 / nbits;

        if (nbufcodes <= 0)
            break;
#if 0
        cout << "0x";

        for (unsigned i = is.gcount(); i > 0; --i)
            Toolbox::hex32(cout, buf[i - 1], 2);

        cout << "\r\n";
#endif
        for (unsigned i = 0, bits = 0; nbufcodes--; ++i, bits += nbits, ++ncodes)
        {
            //let op endianess!
            unsigned *window = (unsigned *)(buf + bits / 8);
            unsigned code = *window >> i * (nbits - 8) % 8 & (1 << nbits) - 1;
            co_yield code;

            if (code == 256)
            {
                nbits = 9, ncodes = 0;
                break;
            }
        }

        if (ncodes == 1U << nbits - 1U && nbits != maxbits)
            ++nbits, ncodes = 0;
    }
}

int main(int argc, char **argv)
{
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    assert(is->get() == 0x1f);
    assert(is->get() == 0x9d);
    int c = is->get();
    assert(c >= 0 && c & 0x80);   //block mode bit is hardcoded in ncompress
#if 0
    Codes1 codes(*is, c & 0x7f);

    for (int code; (code = codes.extract()) != -1;)
        *os << code << "\r\n";
#else
    for (auto code = codes1(*is, c & 0x7f); code;)
        *os << code() << "\r\n";
#endif
    os->flush();
    ifs.close();
    return 0;
}



