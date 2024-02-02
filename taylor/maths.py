#!/usr/bin/pypy3

import math

def int32(n):
    return int(n) & 0xffffffff

def mersenne(seed):
    state = [0] * 624
    f = 1812433253 
    m = 397
    u = 11
    s = 7
    b = 0x9D2C5680 
    t = 15
    c = 0xEFC60000 
    l = 18
    state[0] = seed 
    for i in range(1,624):
        state[i] = int32(f * (state[i - 1] ^ (state[i - 1] >> 30)) + i)
    while True:
        for i in range(624):
            temp = int32((state[i] & (1 << 31)) + (state[(i + 1) % 624] & (1 << 31) - 1))
            temp_shift = temp >> 1
            if temp % 2 != 0:
                temp_shift = temp_shift ^ 0x9908b0df
            state[i] = state[(i + m) % 624] ^ temp_shift
        for i in range(624):
            y = state[i]
            y ^= y >> u
            y ^= (y << s) & b
            y ^= (y << t) & c
            y ^= y >> l
            yield int32(y)

def pi():
    def leibniz(xmax):
        yield 4
        for n in range(1, xmax):
            yield -4 / (n * 4 - 1)
            yield 4 / (n * 4 + 1)
    return sum(leibniz(10**6))

def product(l):
    xsum = 1
    for n in l: xsum *= n
    return xsum

def series(a, b, c):
    x = a
    ret = p = b
    for i in range(1, 20):
        ret += (p := -p * x * x / ((2 * i + c) * 2 * i))
    return ret   

def cos(x):
    return series(x % (2 * math.pi), 1, -1)

def sin(x):
    x = x % (2 * math.pi)
    return series(x, x, 1)

def factorial(n):
    return product(range(2, n + 1))

def e():
    ret = 0
    for n in range(10**3):
        ret += 1 / factorial(n)
    return ret

if __name__ == "__main__":
    print(pi())
    print(e())
    rng = mersenne(124)
    for _ in range(4):
        n = next(rng)
        a = math.cos(n)
        b = cos(n)
        c = abs(a - b)
        print(c)
        a = math.sin(n)
        b = sin(n)
        c = abs(a - b)
        print(c)
    #import numpy
    #a = [sin(x) for x in numpy.arange(-10,10,0.1)]
    #b = [abssin(x) for x in numpy.arange(-10,10,0.1)]
    #import matplotlib.pyplot as plt
    #plt.plot(a)
    #plt.plot(b)
    #plt.show()



