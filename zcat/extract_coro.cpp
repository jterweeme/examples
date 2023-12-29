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
        if (!_full)
        {
            _h();
    
            if (_h.promise().exception_)
                rethrow_exception(_h.promise().exception_);
 
            _full = true;
        }
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

unsigned tell = 0;

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
            tell = _is.tellg();
            if (c == -1) return -1;
            _window |= c << _bits;
        }

        int ret = _window & (1 << n) - 1;
        _window >>= n, _bits -= n, _cnt += n;
        return ret;
    }
};

static Generator<unsigned> codes(istream *is, unsigned maxbits)
{
    BitStream bis(*is);
    unsigned cnt = 0, nbits = 9;

    for (int code; (code = bis.readBits(nbits)) != -1;)
    {
        if (++cnt == 1U << nbits - 1 && nbits != maxbits)
            ++nbits, cnt = 0;

        if (code == 256)
        {
            //cerr << tell << "\r\n";
            for (const unsigned nb3 = nbits << 3; (bis.cnt() - 1U + nb3) % nb3 != nb3 - 1U;)
                bis.readBits(nbits);

            cnt = 0, nbits = 9;
        }

        co_yield code;
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
    unsigned cnt = 0;

    for (auto code = codes(is, c & 0x7f); code;)
    {
        *os << code() << "\r\n";
        ++cnt;
    }

    os->flush();
    cerr << cnt << " codes extracted.\r\n";
    cerr.flush();
    ifs.close();
    return 0;
}



