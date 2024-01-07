#include <iostream>
#include <coroutine>
#include <cstdint>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>

using std::exception_ptr;
using std::convertible_to;
using std::suspend_always;
using std::coroutine_handle;
using std::current_exception;
using std::move;
using std::rethrow_exception;
using std::runtime_error;
using std::forward;
using std::string;


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
};

static int stoi(string &s)
{
    int ret = 0;
    
    for (char c : s)
    {
        if (!isdigit(c))
            break; 

        ret = ret * 10 + (c - '0');
    }

    return ret; 
}   

static bool getline(istream &is, string &s)
{
    s.clear();
    
    for (int c; (c = is.get()) != -1;)
    {
        if (c != '\r' && c != '\n')
            s.push_back(c);
        else if (s.size() > 0)
            return true;
    }

    return false;
}

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(2, 8192);
}

using my::istream;
using my::ostream;
using my::cin;
using my::cout;
using std::cerr;

class BitOutputStream
{
    ostream &_os;
    unsigned _window = 0, _bits = 0;
public:
    uint64_t cnt = 0;
    BitOutputStream(ostream &os) : _os(os) { }

    void write(uint16_t code, unsigned n_bits)
    {
        _window |= code << _bits, cnt += n_bits, _bits += n_bits;
        while (_bits >= 8) flush();
    }

    void flush()
    {
        if (_bits)
        {
            const unsigned bits = std::min(_bits, 8U);
            _os.put(_window & 0xff);
            _window = _window >> bits, _bits -= bits;
        }
    }
};

static Generator<unsigned> codes(istream &is)
{
    for (string s; my::getline(is, s);)
        co_yield my::stoi(s);
}

int main()
{
    ostream *os = &cout;
    BitOutputStream bos(*os);
    bos.write(0x9d1f, 16);
    bos.write(16, 7);
    bos.write(1, 1);
    unsigned cnt = 0, nbits = 9;
    const unsigned bitdepth = 16;

    for (auto codes = ::codes(cin); codes;)
    {
        unsigned code = codes();
        //cerr << code << "\r\n";
        bos.write(code, nbits);
        ++cnt;

        if (code == 256)
        {
            for (; cnt % 8; ++cnt)
                bos.write(0, nbits);

            nbits = 9, cnt = 0;
        }
        
        if (nbits != bitdepth && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    bos.flush();
    os->flush();

    return 0;
}


