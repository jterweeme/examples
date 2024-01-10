#include <coroutine>
#include <exception>
#include <stdexcept>

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


