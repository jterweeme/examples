#include <coroutine>
#include <cstdint>
#include <exception>
#include <concepts>
#include <iostream>

using std::convertible_to;
using std::suspend_always;
using std::coroutine_handle;
using std::exception_ptr;
using std::current_exception;
using std::move;
using std::runtime_error;
using std::forward;
using std::cout;
 
template <typename T> class Generator
{
private:
    bool full_ = false;
 
    void _fill()
    {
        if (!full_)
        {
            h_();

            if (h_.promise().exception_)
                std::rethrow_exception(h_.promise().exception_);
            // propagate coroutine exception in called context
 
            full_ = true;
        }
    }
public:
    struct promise_type
    {
        T value;
        exception_ptr exception_;
        Generator get_return_object() { return Generator(handle_type::from_promise(*this)); }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception_ = current_exception(); }
        void return_void() {}
                                                                             
        template <convertible_to<T> From> suspend_always yield_value(From &&from)
        {
            value = std::forward<From>(from);
            return {};
        }
    };

    using handle_type = coroutine_handle<promise_type>;
    coroutine_handle<promise_type> h_;
    Generator(coroutine_handle<promise_type> h) : h_(h) {}
    ~Generator() { h_.destroy(); }
    explicit operator bool() { _fill(); return !h_.done(); }

    T operator()()
    {
        _fill();
        full_ = false; 
        return move(h_.promise().value);
    }
};
 
Generator<std::uint64_t> fibonacci_sequence(unsigned n)
{
    if (n == 0)
        co_return;
 
    if (n > 94)
        throw runtime_error("Too big Fibonacci sequence. Elements would overflow.");
 
    co_yield 0;
 
    if (n == 1)
        co_return;
 
    co_yield 1;
 
    if (n == 2)
        co_return;
 
    std::uint64_t a = 0;
    std::uint64_t b = 1;
 
    for (unsigned i = 2; i < n; ++i)
    {
        std::uint64_t s = a + b;
        co_yield s;
        a = b;
        b = s;
    }
}
 
int main()
{
    auto gen = fibonacci_sequence(10); // max 94 before uint64_t overflows
 
    for (int j = 0; gen; ++j)
        std::cout << "fib(" << j << ")=" << gen() << '\n';
#if 0
    for (auto n : fibonacci_sequence(10))
        std::cout << n << "\r\n";
#endif
}


