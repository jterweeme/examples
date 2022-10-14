#include <iostream>
#include <iomanip>
#include <random>
#include <math.h>

static constexpr double PI = 3.1415926535;

double doublemod(double a, double b)
{
    return a - b * int(a / b);
}

double mycos(double x)
{
    if (x < 0)
        x *= -1;

    x = doublemod(x, 2*PI);
    char sign = 1;

    if (x > PI)
    {
        x -= PI;
        sign = -1;
    }

    double ret = 1;
    uint64_t fact = 2;
    uint64_t j = 3;
    double base = x;

    for (uint32_t i = 0; i < 4; ++i)
    {
        base *= x;
        ret -= base / fact;
        fact *= j++;
        fact *= j++;
        base *= x * x;
        ret += base / fact;
        base *= x;
        fact *= j++;
        fact *= j++;
    }

    return ret * sign;
}

double cosinus(double x)
{
    x = fabs(x);
    x = doublemod(x, 2*PI);
    char sign = 1;

    if (x > PI)
        x -= PI, sign = -1;

    int p = 0;
    double s = 1.0, t = 1.0;

    while(fabs(t/s) > 0.0001)
        s += (t = (-t * x * x) / ((2 * ++p - 1) * (2 * p)));
    
    return s * sign;
}

double mysine(double x)
{
    return 0.0;
}

void myfunction(double x)
{
    std::cout << x << "\r\n"
              << cos(x) << "\r\n"
              << mycos(x) << "\r\n" << cosinus(x) << "\r\n\r\n";
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


