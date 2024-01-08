//Usage: ./extractc archive.Z | ./lzw > archive

#include <coroutine>
#include <cassert>
#include <cstdint>
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

namespace my
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
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(2, 8192);
}

using my::ostream;
using my::istream;
using my::ifstream;
using my::cin;
using my::cout;
using my::cerr;

class ByteStack
{
    std::vector<char> _stack;
public:
    void push(char c) { _stack.push_back(c); }
    char top() const { return _stack.back(); }
    void pop_all(ostream &os) { for (; _stack.size(); _stack.pop_back()) os.put(top()); }
};

class Dictionary
{
    unsigned _cap;
    uint16_t *_codes;
    char *_bytes;
    unsigned _pos = 0;
public:
    Dictionary(unsigned cap)
      : _cap(cap), _codes(new uint16_t[cap - 256]), _bytes(new char[cap - 256]) { }

    void lookup(ByteStack &s, uint16_t code)
    {
        for (; code >= 256U; code = _codes[code - 256])
            s.push(_bytes[code - 256]);

        s.push(code);
    }

    void store(unsigned code, uint8_t c)
    {
        if (_pos + 256 < _cap)
            _codes[_pos] = code, _bytes[_pos] = c, ++_pos;
    }

    ~Dictionary() { delete[] _codes; delete[] _bytes; }
    auto size() const { return _pos + 256; }
    void clear() { _pos = 0; }
};

static Generator<unsigned> codes(istream &is)
{
    unsigned n = 0;
    bool flag = false;

    for (int c; (c = is.get()) != -1;)
    {
        if (isdigit(c))
        {
            flag = true;
            n = n * 10 + c - '0';
        }
        else
        {
            if (flag)
                co_yield n;

            flag = false;
            n = 0;
        }
    }
}

static void
lzw(unsigned dictcap, ostream &os, Generator<unsigned> codes)
{
    Dictionary dict(dictcap);
    ByteStack stack;
    unsigned oldcode = 0;
    char finchar = 0;

    while (codes)
    {
        unsigned newcode, c;
        newcode = c = codes();
        assert(c <= dict.size());

        if (c == 256)
        {
            dict.clear();
            continue;
        }

        if (c == dict.size())
            stack.push(finchar), c = oldcode;

        dict.lookup(stack, c);
        dict.store(oldcode, finchar = stack.top());
        oldcode = newcode;
        stack.pop_all(os);
    }
}

int main(int argc, char **argv)
{
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;
    
    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    ::lzw(1 << 16, *os, ::codes(*is));
    os->flush();
    ifs.close();
    return 0;
}


