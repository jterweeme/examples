//This is a comment
//I love comments

#include <cassert>
#include <cstdint>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

using std::string;
using std::vector;

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

#if 1
using my::ostream;
using my::istream;
using my::ifstream;
using my::cin;
using my::cout;
using my::cerr;
#else
using std::ostream;
using std::istream;
using std::ifstream;
using std::cout;
using std::cin;
#endif

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

    void append(unsigned code, uint8_t c)
    {
        if (_pos + 256 < _cap)
            _codes[_pos] = code, _bytes[_pos] = c, ++_pos;
    }

    ~Dictionary() { delete[] _codes; delete[] _bytes; }
    auto size() const { return _pos + 256; }
    void clear() { _pos = 0; }
};

class LZW
{
    unsigned _oldcode = 0;
    uint8_t _finchar;
    Dictionary _dict;
    ostream &_os; 
    ByteStack _stack;
public:
    LZW(unsigned dictcap, ostream &os) : _dict(dictcap), _os(os) { }

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

int main(int argc, char **argv)
{
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;
    
    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    LZW lzw(1 << 16, *os);

    for (string s; my::getline(*is, s);)
        lzw.code(std::stoi(s));

    os->flush();
    ifs.close();
    return 0;
}


