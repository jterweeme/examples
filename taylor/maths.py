#!/usr/bin/python3

import math, numpy
from matplotlib import pyplot

def sign(x):
    return -1 if x < 0 else 1

def prod(l):
    xsum = 1
    for n in l:
        xsum *= n
    return xsum

def factorial(n):
    return prod(range(2, n + 1))

def xpow_int(x, xp):
    ret = prod([x] * abs(xp))
    return ret if xp >= 0 else 1 / ret

def atan0(x, n_terms = 99):
    assert -1 <= x <= 1
    ret = p = x
    for n in range(3, n_terms, 2):
        ret += (p := -p * x * x) / n
    return ret

#3.141592653589793238462643383279502884197...
def pi():
    return 4 * (4 * atan0(1/5) - atan0(1/239))

#6.283185307179586...
def tau():
    return pi() * 2

def atan(x, nterms = 99):
    if -1 < x < 1:
        return atan0(x)
    p = -x
    ret = pi()
    ret /= -2 if x < -1 else 2
    for i in range(1, nterms * 2 + 1, 2):
        ret += 1 / (i * p)
        p = -p * x * x
    return ret

def atanh(x, n_terms = 99):
    ret = p = x
    for n in range(3, n_terms, 2):
        ret += (p := p * x * x) / n
    return ret

def log1p(x):
    return 2 * atanh(x/(2+x))

def log(x):
    return atanh((x * x - 1) / (x * x + 1)) if x < 1 else log1p(x - 1)

def exp(x, nterms = 99):
    fact = ret = p = 1
    for n in range(1, nterms):
        ret += (p := p * x) / (fact := fact * n)
    return ret

def expm1(x):
    return exp(x) - 1

#2.7182818284590452353602874713527...
def e():
    return exp(1)

def xpow(x, xp):
    return exp(xp * log(x)) if type(xp) == float else xpow_int(xp, xp)

def cos(x):
    return exp((x % tau()) *1j).real

def sin(x):
    return exp((x % tau()) *1j).imag

def sinh(x):
    foo = exp(x)
    return (foo - 1/foo) / 2

def cosh(x):
    foo = exp(x)
    return (foo + 1/foo) / 2

def tanh(x):
    foo = exp(2*x)
    return (foo - 1) / (foo + 1)

def sech(x):
    foo = exp(x)
    return 2 / (foo + 1/foo)

def csch(x):
    foo = exp(x)
    return 2 / (foo - 1/foo)

def int32(n):
    return int(n) & 0xffffffff

def exp2(xp):
    return xpow(2, xp)

def sqrt(x):
    return exp(log(x) / 2) if x > 0 else 0

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

def asin(x):
    return atan(x/sqrt(1-x*x)) if abs(x) != 1 else sign(x) * pi() / 2

def acos(x):
    return atan(-1/sqrt(1-x*x))

def sec(x):
    return 1 / cos(x)

def csc(x):
    return 1 / sin(x)

def tan(x):
    return sin(x) / cos(x)

def cot(x):
    return 1 / tan(x)

def a007680(nterms = 20):
    for i in range(nterms):
        yield (2*i+1)*math.factorial(i)

def mersenne(seed):
    state = [seed] + [0] * 623
    f, m, u, s, b, t, c, l = 1812433253, 397, 11, 7, 0x9D2C5680, 15, 0xEFC60000, 18
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

def test(capt, c, a, b):
    error = 1 if a == b else min(abs(a), abs(b)) / max(abs(a), abs(b))
    if math.isnan(a) or math.isnan(b):
        error = 0
    pct = error * 100
    color = 92 if error > 0.99 else 91
    print(f"\033[{color}m{capt} {c} {a} {b} {pct}%\033[0m")

def test2(c):
    test("atan", c, math.atan(c), atan(c))
    test("exp", c, math.exp(c), exp(c))
    test("expm1", c, math.expm1(c), expm1(c))
    test("sqrt", c, math.sqrt(c), sqrt(c))
    test("log", c, math.log(c), log(c))

def test3(c):
    test("sin", c, math.sin(c), sin(c))
    test("cos", c, math.cos(c), cos(c))
    test("tan", c, math.tan(c), tan(c))
    test("atan", c, math.atan(c), atan(c))
    #test("csc", c, math.csc(c), csc(c))

def test1(c):
    test("atanh", c, math.atanh(c), atanh(c))
    test("asin", c, math.asin(c), asin(c))
    test2(c)

if __name__ == "__main__":
    test1(0.5)
    test2(2)
    test2(40)
    test("acos", -0.9999, math.acos(-0.9999), acos(-0.9999))
    test("asin", 1, math.asin(1), asin(1))
    test("exp", 2.5, math.exp(2.5), exp(2.5))
    test("sqrt", 0, math.sqrt(0), sqrt(0))
    test("sqrt", 2, math.sqrt(2), sqrt(2))
    test("log", 2, math.log(2), log(2))
    test("pi", 1, math.pi, pi())
    test("e", 1, math.e, e())
    rng = mersenne(124)
    for _ in range(4):
        n = next(rng)
        test3(n)
        #test(n, math.exp(n), exp(n))

    a = [atan(x) for x in numpy.arange(-0.999, 0.999,0.01)]
    #a = [sin_cordic(x) for x in numpy.arange(-10, 10, 0.1)]
    b = [asin(x) for x in numpy.arange(-0.999,0.999,0.01)]
    #b = [abssin(x) for x in numpy.arange(-10,10,0.1)]
    #c = [acos(x) for x in numpy.arange(-0.999,0.999,0.01)]
    pyplot.plot(a)
    pyplot.plot(b)
    #pyplot.plot(c)
    pyplot.show()

def sqrt_babylonian(num):
    ret = 1
    for term in range(10):
        ret = (ret + num / ret) / 2
    return ret

def binary(num, root):
    guess = num / 2
    for term in range(1, 100):
        exp = xpow(guess, root)
        if exp > num:
            guess -= num / (1 << term)
        elif exp < num:
            guess += num / (1 << term)
        else:
            break
    return guess

def pi_madhava(n_terms = 40):
    ret = 0
    for k in range(n_terms):
        ret += xpow(-3, -k) / (2 * k + 1)
    return ret * sqrt(12)

def sqrt_(x):
    return sqrt_babylonian(x)
    return binary(x, 2)

def sin_(x, nterms = 20):
    x = x % tau()
    ret = p = x
    for i in range(1, nterms):
        ret += (p := -p * x * x / ((2 * i + 1) * 2 * i))
    return ret

def cos_(x, nterms = 20):
    x = x % tau()
    ret = p = 1
    for i in range(1, nterms):
        ret += (p := -p * x * x / ((2 * i -1) * 2 * i))
    return ret


