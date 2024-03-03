#!/usr/bin/python3

import math, numpy
from math import radians
from matplotlib import pyplot

theta_table = [math.atan2(1, 1<<i) for i in range(16)]

K_n = 1.0
for i in range(len(theta_table)):
    K_n *= 1 / math.sqrt(1 + 2 ** (-2 * i))

def cordic(alpha):
    y = theta = 0.0
    x = 1.0
    P2i = 1  #This will be 2**(-i) in the loop below
    for arc_tangent in theta_table:
        sigma = +1 if theta < alpha else -1
        theta += sigma * arc_tangent
        x, y = x - sigma * y * P2i, sigma * P2i * x + y
        P2i /= 2
    return x * K_n, y * K_n

if __name__ == "__main__":
    """
    Print a table of computed sines and cosines, from -90° to +90°, in steps of 15°,
    comparing against the available math routines.
    """
    print("  x       sin(x)     diff. sine     cos(x)    diff. cosine ")
    for x in range(-90, 91, 15):
        cos_x, sin_x = cordic(radians(x))
        diff_sin = sin_x - math.sin(radians(x))
        diff_cos = cos_x - math.cos(radians(x))
        print(f"{x:+05.1f}°  {sin_x:+.8f} ({diff_sin:+.8f}) {cos_x:+.8f} ({diff_cos:+.8f})")

hpi = math.pi / 2
a = [cordic(x)[0] for x in numpy.arange(-hpi, +hpi, 0.1)]
b = [cordic(x)[1] for x in numpy.arange(-hpi, +hpi, 0.1)]
pyplot.plot(a)
pyplot.plot(b)
pyplot.show()



