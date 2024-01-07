//This is a comment
//I love comments

#include <cstdint>
#include <cassert>
#include <coroutine>
#include <iostream>

union Fcode
{
    uint32_t code = 0;
    struct
    {
        uint16_t c;
        uint16_t ent;
    } e;
};

using std::exception_ptr; 
using std::convertible_to; 
using std::suspend_always;
using std::coroutine_handle;
using std::current_exception;
using std::move;
using std::rethrow_exception;
using std::runtime_error;
using std::forward;
using std::cin;
using std::cout;
using std::istream;
using std::ostream;

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

static Generator<unsigned> codify(istream &is)
{
    static constexpr unsigned HSIZE = 69001;
    uint16_t codetab[HSIZE];
    std::fill(codetab, codetab + HSIZE, 0);
    Fcode fcode;
    fcode.e.ent = is.get();
    unsigned n_bits = 9, free_ent = 257;
    unsigned extcode = (1 << n_bits) + 1, htab[HSIZE];

    for (int byte; (byte = is.get()) != -1;)
    {
        if (free_ent >= extcode && fcode.e.ent < 257)
        {
            if (n_bits < 16)
                ++n_bits, extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << 16;
            else
            {
                std::fill(codetab, codetab + HSIZE, 0);
                co_yield 256;
                n_bits = 9, free_ent = 257;
                extcode = n_bits < 16 ? (1 << n_bits) + 1 : 1 << n_bits;
            }
        }

        fcode.e.c = byte;
        unsigned fc = fcode.code;
        unsigned hp = fcode.e.c << 8 ^ fcode.e.ent;
        bool hfound = false;

        while (codetab[hp])
        {
            if (htab[hp] == fc)
            {
                fcode.e.ent = codetab[hp];
                hfound = true;
                break;
            }

            if ((hp += hp + 1) >= HSIZE)
                hp -= HSIZE;
        }

        if (!hfound)
        {
            co_yield fcode.e.ent;
            fc = fcode.code;
            fcode.e.ent = fcode.e.c;
            codetab[hp] = free_ent++, htab[hp] = fc;
        }
    }

    co_yield fcode.e.ent;
}

int main(int argc, char **argv)
{
    auto os = &cout;
    BitOutputStream bos(*os);
    bos.write(0x9d1f, 16);
    bos.write(16, 7);
    bos.write(1, 1);
    unsigned cnt = 0, nbits = 9;
    
    for (auto codes = codify(cin); codes;)
    {
        unsigned code = codes();
        bos.write(code, nbits);
        ++cnt;

        if (code == 256)
        {
            while (cnt++ % 8)
                bos.write(0, nbits);

            nbits = 9, cnt = 0;
        }

        if (nbits != 16 && cnt == 1U << nbits - 1)
            ++nbits, cnt = 0;
    }

    bos.flush();
    os->flush();
    return 0;
}


