#include <unistd.h>
#include <fcntl.h>
#include <cstdint>

namespace mystl
{   
class istream
{   
private:
    uint32_t _cap, _head = 0, _tail = 0;
    uint8_t *_buf;
    ssize_t _gcount = -1;
protected:
    int _fd; 
public:
    ~istream() { delete[] _buf; }
    ssize_t gcount() const { return _gcount; }
    
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
    
    void read(char *buf, unsigned n)
    {
        _gcount = 0;
    
        for (int c; n-- && (c = get()) != -1;)
            buf[_gcount++] = c;
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
    ~ostream() { flush(); delete[] _buf; }
    inline void put(char c) { if (_pos > _cap) flush(); _buf[_pos++] = c; }
    void flush() { ::write(_fd, _buf, _pos), _pos = 0; }
    inline ostream& operator<<(const char *s) { while (*s) put(*s++); return *this; }
    void write(char *buf, unsigned len) { for (unsigned i = 0; i < len; ++i) put(buf[i]); }
};

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(2, 8192);
}


