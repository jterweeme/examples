#include <iostream>
#include <iomanip>
#include <random>
#include <iomanip>
#include <math.h>

using std::cout;

static constexpr auto PI = M_PI;
static constexpr auto DPI = M_PI * 2;

static auto cosinus(double x)
{
    x = fmod(fabs(x), DPI);
    char sign = 1;

    if (x > PI)
        x -= PI, sign = -1;

    double s = 1.0, t = 1.0;

    for (unsigned p = 1; fabs(t / s) > 1e-5; ++p)
        s += (t = -t * x * x / ((2 * p - 1) * 2 * p));

    return s * sign;
}

static auto sine(double x)
{
    x = fmod(x, DPI);
    double res = 0, pow = x, fact = 1;

    for (int i = 1; i < 20; ++i)
    {
        res += pow / fact;
        pow *= -1 * x * x;
        fact *= 2 * i * (2 * i + 1);
    }

    return res;
}

static auto myfunction(double x)
{
    auto mycos = ::cosinus(x);
    auto stdcos = ::cos(x);
    auto diffcos = abs(mycos - stdcos);
    auto mysin = ::sine(x);
    auto stdsin = ::sin(x);
    auto diffsin = abs(mysin - stdsin);

    cout << "cos\r\n"
         << x << "\r\n"
         << std::setprecision(10) << stdcos << "\r\n"
         << mycos << "\r\n" << diffcos << "\r\n\r\n";

    cout << "sin\r\n" << x << "\r\n"
         << stdsin << "\r\n"
         << mysin << "\r\n" << diffsin << "\r\n\r\n";
}

int main()
{
    double lower_bound = -1000;
    double upper_bound = 1000;
    std::uniform_real_distribution<double> unif(lower_bound, upper_bound);
    std::default_random_engine re;
    for (uint32_t i = 0; i < 50; ++i)
        myfunction(unif(re));

    std::cout.flush();
    return 0;
}


