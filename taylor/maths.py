#!/usr/bin/python3

import math, numpy
import matplotlib.pyplot as plt

def atan(x):
    return math.atan(x)

def ln(x):
    return math.log(x)

def int32(n):
    return int(n) & 0xffffffff

def xpow(x, exp):
    return x**exp
    ret = x
    for _ in range(int(abs(exp)) - 1):
        ret *= x
    return ret if exp >= 0 else 1 / ret

def exp2(x):
    return 2**x

#2.7182818284590452353602874713527
def e(nterms = 999):
    ret = fact = 1
    for n in range(1, nterms):
        ret +=1 / (fact := fact * n)
    return ret

def sqrt_babylonian(num):
    ret = 1
    for term in range(10):
        ret = (ret + num / ret) / 2
    return ret

def binary(num, root):
    guess = num / 2
    for term in range(1, 100):
        exp = guess ** root
        if exp > num:
            guess -= num / (1 << term)
        elif exp < num:
            guess += num / (1 << term)
        else:
            break
    return guess

def sqrt(x):
    return sqrt_babylonian(x)
    return binary(x, 2)

def pi_madhava(n_terms = 40):
    ret = 0
    for k in range(n_terms):
        ret += (-3)**-k / (2*k+1)
    return ret * sqrt(12)

#3.141592653589793238462643383279502884197
def pi():
    return 4 * (4 * atan(1/5) - atan(1/239))

def tau():
    return pi() * 2

def prod(l):
    xsum = 1
    for n in l: xsum *= n
    return xsum

def factorial(n):
    return prod(range(2, n + 1))

def fibonacci(xmax, term1 = 1, term2 = 2):
    yield term1
    yield term2
    while term1 + term2 <= xmax:
        yield term1 + term2
        term1, term2 = term2, term1 + term2

def reverse(n, base = 10):
    temp, rev = n, 0
    while temp != 0:
        rev = rev * base + temp % base
        temp = temp // base
    return rev

def ispalindrome(n, base = 10):
    return n == reverse(n, base)

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

def series(a, b, c, nterms = 20):
    x = a
    ret = p = b
    for i in range(1, nterms):
        ret += (p := -p * x * x / ((2 * i + c) * 2 * i))
    return ret   

def cos(x):
    return series(x % tau(), 1, -1)

def sec(x):
    return 1 / cos(x)

def sin(x):
    x = x % tau()
    return series(x, x, 1)

def tan(x):
    return sin(x) / cos(x)

def exp(x):
    return e()**x

def sinh(x):
    foo = exp(x)
    return (foo - 1/foo) / 2

def tanh(x):
    foo = exp(2*x)
    return (foo - 1) / (foo + 1)

if __name__ == "__main__":
    print(math.sqrt(2))
    print(sqrt(2))
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

    a = [sin(x) for x in numpy.arange(-10,10,0.1)]
    #a = [sin_cordic(x) for x in numpy.arange(-10, 10, 0.1)]
    b = [math.sin(x) for x in numpy.arange(-10,10,0.1)]
    #b = [abssin(x) for x in numpy.arange(-10,10,0.1)]
    plt.plot(a)
    #plt.plot(b)
    #plt.show()



