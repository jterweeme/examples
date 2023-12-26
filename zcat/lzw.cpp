//This is a comment
//I love comments

#define FAST

#include <cassert>
#include <cstdint>
#include <vector>
#include <string>

#ifdef FAST
#include <unistd.h>
#include <fcntl.h>

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

static bool getline(istream &is, std::string &s)
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
#else
#include <iostream>
#include <fstream>

using std::ostream;
using std::istream;
using std::ifstream;
using std::cout;
using std::cin;
using std::getline;
#endif

class PrintStack
{
    std::vector<char> _stack;
public:
    void push_back(char c) { _stack.push_back(c); }
    void print(ostream &os) { for (; _stack.size(); _stack.pop_back()) os.put(_stack.back()); }
};

class LZW
{
    const unsigned _maxbits;
    unsigned _oldcode = 0;
    char _finchar;
    std::vector<std::pair<unsigned, char>> _dict;
    ostream &_os; 
    PrintStack _stack;
public:
    LZW(unsigned maxbits, ostream &os) : _maxbits(maxbits), _os(os) { }

    inline void code(const unsigned in)
    {
        assert(in <= _dict.size() + 256);
        auto c = in;

        if (in == 256)
        {
            _dict.clear();
            return;
        }

        if (c == _dict.size() + 256)
        {
            _stack.push_back(_finchar);
            c = _oldcode;
        }

        for (; c >= 256U; c = _dict[c - 256].first)
            _stack.push_back(_dict[c - 256].second);

        _os.put(_finchar = c);

        if (_dict.size() + 256 < 1U << _maxbits)
            _dict.push_back(std::pair<unsigned, char>(_oldcode, _finchar));

        _oldcode = in;
        _stack.print(_os);
    }
};

int main(int argc, char **argv)
{
    istream *is = &cin;
    ostream *os = &cout;
    ifstream ifs;
    
    if (argc > 1)
        ifs.open(argv[1]), is = &ifs;

    LZW lzw(16, *os);

    for (std::string s; getline(*is, s);)
        lzw.code(std::stoi(s));

    os->flush();
    ifs.close();
    return 0;
}


