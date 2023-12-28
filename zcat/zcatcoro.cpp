//This is a comment
//I love comments

//zcatpp (zcat c++)

#include <cassert>
#include <cstdint>
#include <exception>
#include <coroutine>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

using std::vector;
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
        exception_ptr exception;
        T value;
        suspend_always initial_suspend() { return {}; }
        suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception = current_exception(); }
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
    
            if (_h.promise().exception)
                rethrow_exception(_h.promise().exception);

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
    uint32_t _cap, _head = 0, _tail = 0;
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
    uint32_t _cap, _pos = 0;
    char *_buf;
public:
    ostream(int fd, uint32_t capacity) : _fd(fd), _cap(capacity), _buf(new char[capacity]) { }
    ~ostream() { delete[] _buf; }
    void put(char c) { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
}

#if 1
using fast::istream;
using fast::ostream;
using fast::ifstream;
using fast::cin;
using fast::cout;
#else
using std::istream;
using std::ostream;
using std::ifstream;
using std::cin;
using std::cout;
#endif

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

class ByteStack
{
    vector<char> _stack;
public:
    void push(char c) { _stack.push_back(c); }
    char top() const { return _stack.back(); }
    void pop_all(ostream &os) { for (; _stack.size(); _stack.pop_back()) os.put(top()); }
};

class Dictionary
{
    unsigned _cap;
    uint16_t *_codes;
    uint8_t *_chars;
    unsigned _pos = 0;
public:
    Dictionary(unsigned maxbits)
      : _cap(1 << maxbits), _codes(new uint16_t[_cap - 256]), _chars(new uint8_t[_cap - 256]) { }

    void lookup(ByteStack &s, uint16_t code)
    {
        for (; code >= 256U; code = _codes[code - 256])
            s.push(_chars[code - 256]);

        s.push(code);
    }

    void append(unsigned code, uint8_t c)
    {
        if (_pos + 256 < _cap)
            _codes[_pos] = code, _chars[_pos] = c, ++_pos;
    }

    ~Dictionary() { delete[] _codes; delete[] _chars; }
    auto size() const { return _pos + 256; }
    void clear() { _pos = 0; }
};

class LZW
{
    unsigned _oldcode = 0;
    char _finchar;
    ostream &_os;
    Dictionary _dict;
    ByteStack _stack;
public:
    LZW(unsigned maxbits, ostream &os) : _os(os), _dict(maxbits) { }

    inline void code(const unsigned in)
    {
        assert(in <= _dict.size());
        auto c = in;

        if (in == 256)
        {
            _dict.clear();
            return;
        }

        if (c == _dict.size())
            _stack.push(_finchar), c = _oldcode;

        _dict.lookup(_stack, c);
        _finchar = _stack.top();
        _dict.append(_oldcode, _finchar);
        _oldcode = in;
        _stack.pop_all(_os);
    }
};

static Generator<unsigned> codes(istream &is, unsigned maxbits)
{
    assert(maxbits >= 9 && maxbits <= 16);
    BitStream bis(is);
    unsigned cnt = 0, nbits = 9;

    for (int code; (code = bis.readBits(nbits)) != -1;)
    {
        //read 256 9-bit codes, 512 10-bit codes, 1024 11-bit codes, etc
        if (++cnt == 1U << nbits - 1 && nbits != maxbits)
            ++nbits, cnt = 0;

        if (code == 256)
        {
            //other max. bits not working yet :S
            assert(maxbits == 13 || maxbits == 15 || maxbits == 16);

            //cumbersome padding formula
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
    ostream * const os = &cout;
    ifstream ifs;

    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    assert(is->get() == 0x1f);
    assert(is->get() == 0x9d);
    int c = is->get();
    assert(c >= 0 && c & 0x80);   //block mode bit is hardcoded in ncompress
    const unsigned maxbits = c & 0x7f;
    LZW lzw(maxbits, *os);

    for (auto code = codes(*is, c & 0x7f); code;)
        lzw.code(code());

    os->flush();
    ifs.close();
    return 0;
}


