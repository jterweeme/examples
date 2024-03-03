#include <stdio.h>
#include <stdint.h>
#include <math.h>
 
float f(float n) {
    n = 1.0f / n;
    long i;
    float x, y;
 
    x = n * 0.5f;
    y = n;
    i = *(long *)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *)&i;
    y = y * (1.5f - (x * y * y));
 
    return y;
}

float sqrt_approx(float z)
{
    union { float f; uint32_t i; } val = {z};	/* Convert type, preserving bit pattern */
    val.i -= 1 << 23;	/* Subtract 2^m. */
    val.i >>= 1;		/* Divide by 2. */
    val.i += 1 << 29;	/* Add ((b + 1) / 2) * 2^m. */
    return val.f;		/* Interpret again as float */
}

int main()
{
    printf("%f\r\n", f(55.3));
    printf("%f\r\n", sqrt_approx(55.3));
    printf("%f\r\n", sqrt(55.3));
    return 0;
}


