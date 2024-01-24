#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <concepts>
#include <bit>
#include <array>
#include <algorithm>

namespace mystd
{
template<std::integral T>
constexpr T byteswap(T value) noexcept
{
    static_assert(std::has_unique_object_representations_v<T>, 
                  "T may not have padding bits");
    auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
    std::ranges::reverse(value_representation);
    return std::bit_cast<T>(value_representation);
}

//https://en.cppreference.com/w/cpp/algorithm/copy
template <class InputIt, class OutputIt>
OutputIt copy(InputIt first, InputIt last, OutputIt d_first)
{
    for (; first != last; (void)++first, (void)++d_first)
        *d_first = *first;
 
    return d_first;
}

//static double inline fabs(double d) { return d > 0 ? d : -d; }
static void *my_malloc(size_t size) { return new uint8_t[size]; }
static void my_free(void *ptr) { delete[] (uint8_t *)ptr; }

static void *my_realloc(void *ptr, size_t oldsize, size_t newsize)
{
    uint8_t *pptr = (uint8_t *)ptr, *tmp = (uint8_t *)my_malloc(newsize);
    copy(pptr, pptr + oldsize, tmp);
    my_free(ptr);
    return (void *)tmp;
}

//https://en.cppreference.com/w/cpp/algorithm/fill
template<class ForwardIt, class T>
void fill(ForwardIt first, ForwardIt last, const T& value)
{
    for (; first != last; ++first)
        *first = value;
}

class string
{
public:
    char *_buf;
    unsigned _capacity;
    unsigned _p = 0;
    string() { _capacity = 2; _buf = (char *)my_malloc(_capacity); }
    ~string() { my_free(_buf); }

    void push_back(char c)
    {
        if (_p == _capacity)
            _buf = (char *)my_realloc(_buf, _capacity, _capacity * 2), _capacity *= 2;

        _buf[_p++] = c;
    }
};

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

class special { };

special endl;

class ostream
{
    int _fd;
    uint32_t _cap, _pos = 0;
    char *_buf;
public:
    ostream(int fd = -1, uint32_t capacity = 0)
      : _fd(fd), _cap(capacity), _buf(new char[capacity]) { }

    ~ostream() {
        flush(); delete[] _buf;
    }

    inline void put(char c) {
        if (_pos > _cap) flush();
        _buf[_pos++] = c;
    }

    void write(char *buf, unsigned len) {
        for (unsigned i = 0; i < len; ++i) put(buf[i]);
    }

    void flush() {
        ::write(_fd, _buf, _pos), _pos = 0;
    }

    virtual ostream& operator<<(char c) {
        put(c);
        return *this;
    }

    virtual inline ostream& operator<<(const char *s) {
        while (*s) put(*s++);
        return *this;
    }

    virtual inline ostream& operator<<(string s) {
        this->write(s._buf, s._p);
        return *this;
    }

    virtual inline ostream& operator<<(special) {
        put('\r');
        put('\n');
        flush();
        return *this;
    }

    virtual inline ostream& operator<<(unsigned n)
    {
        unsigned p = 32;
        char buf[p];
        
        while (n)
            buf[--p] = n % 10 + '0', n /= 10;

        this->write(buf + p, 32 - p);
        return *this;
    }

};

static istream cin(0, 8192);
static ostream cout(1, 8192);
static ostream cerr(2, 8192);
}




