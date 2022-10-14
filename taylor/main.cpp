#include <iostream>
#include <iomanip>
#include <random>
#include <math.h>

static constexpr double PI = 3.1415926535;

template <class T> static T powi(T base, uint64_t e)
{
    T ret = base;

    for (uint64_t i = 1; i < e; ++i)
        ret *= base;

    return e > 0 ? ret : 1;
}

static double powdi(double base, uint64_t e) {
    return powi<double>(base, e);
}

//2) 4 - 16 - 64 - 256 - 1024

double cos_taylor_literal_6terms_naive(double x, uint32_t n)
{
    double ret = 1;
    uint64_t fact = 2;
    uint64_t j = 3;

    for (uint32_t i = 0; i < n; ++i)
    {
        ret -= powdi(x, 2 + i * 4) / fact;
        fact *= j++;
        fact *= j++;
        ret += powdi(x, 4 + i * 4) / fact;
        fact *= j++;
        fact *= j++;
    }

    return ret;
}

double doublemod(double a, double b)
{
    return a - b * int(a / b);
}

double mycos(double x)
{
    if (x < 0)
        x *= -1;

    double ret = doublemod(x, 2*PI);
    char sign = 1;

    if (ret > PI)
    {
        ret -= PI;
        sign = -1;
    }
    return sign * cos_taylor_literal_6terms_naive(ret, 3);
}

double mysine(double x)
{
    return 0.0;
}

void myfunction(double x)
{
    std::cout << x << "\r\n" << cos(x) << "\r\n" << mycos(x) << "\r\n\r\n";
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


