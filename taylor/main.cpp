#include <iostream>
#include <iomanip>
#include <random>
#include <math.h>

static constexpr double PI = 3.1415926535;

double cosinus(double x)
{
    x = fabs(x);
    x = fmod(x, 2*PI);
    char sign = 1;

    if (x > PI)
        x -= PI, sign = -1;

    int p = 0;
    double s = 1.0, t = 1.0;

    while(fabs(t/s) > 0.0001)
        s += (t = (-t * x * x) / ((2 * ++p - 1) * (2 * p)));
    
    return s * sign;
}

double sine(double x)
{
    x = fmod(x, 2*PI);

    double res=0, pow=x, fact=1;

    for (int i=0; i<20; ++i)
    {
        res+=pow/fact;
        pow*=-1*x*x;
        fact*=(2*(i+1))*(2*(i+1)+1);
    }

    return res;
}

void myfunction(double x)
{
    std::cout << x << "\r\n"
              << cos(x) << "\r\n"
              << cosinus(x) << "\r\n\r\n";

    std::cout << x << "\r\n"
              << sin(x) << "\r\n"
              << sine(x) << "\r\n\r\n";
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


