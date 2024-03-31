#!/usr/bin/python3

import math, numpy


def sign(x):
    return -1 if x < 0 else 1

def prod(l):
    xsum = 1
    for n in l:
        xsum *= n
    return xsum

def exp2_int(xp):
    ret = 1 << abs(xp)
    return ret if xp >= 0 else 1/ret

#depends: prod()
def factorial(n):
    return prod(range(2, n + 1))

#depends: prod()
def xpow_int(x, xp):
    ret = prod([x] * abs(xp))
    return ret if xp >= 0 else 1 / ret

#loop
def atan0(x, n_terms = 99):
    assert abs(x) <= 1
    ret = p = x
    for n in range(3, n_terms, 2):
        ret += (p := -p * x * x) / n
    return ret

#depends: atan0()
#3.141592653589793238462643383279502884197...
def pi():
    return 4 * (4 * atan0(1/5) - atan0(1/239))

#depends: atan0()
#6.283185307179586...
def tau():
    return 8 * (4 * atan0(1/5) - atan0(1/239))

#loop, depends: atan0()
def atan(x, nterms = 99):
    if abs(x) < 1:
        return atan0(x)
    p = -x
    ret = pi()
    ret /= -2 if x < -1 else 2
    for i in range(1, nterms * 2 + 1, 2):
        ret += 1 / (i * p)
        p = -p * x * x
    return ret

#loop
def atanh(x, n_terms = 1999):
    if isinstance(x, complex) == False:
        assert(abs(x) < 1)
    ret = p = x
    for n in range(3, n_terms, 2):
        ret += (p := p * x * x) / n
    return ret

def log1m(x, nterms = 99):
    p = x
    ret = -x
    for n in range(2, nterms):
        ret -= (p := p * x) / n
    return ret

def log1p(x, nterms = 99):
    #return 2 * atanh(x/(2+x))
    ret = p = x
    for n in range(2, nterms):
        ret += (p := -p * x) / n
    return ret

def log(x):
    return atanh((x * x - 1) / (x * x + 1)) if x < 1 else log1p(x - 1)

def atanh_(x):
    return numpy.log((1+x)/(1-x))/2

#loop
def sin(x, nterms = 20):
    if isinstance(x, complex) == False:
        x = x % tau()
    ret = p = x
    for i in range(1, nterms):
        ret += (p := -p * x * x / ((2 * i + 1) * 2 * i))
    return ret
    return exp(fmod(x, tau()) *1j).imag

#loop
def cos(x, nterms = 20):
    if isinstance(x, complex) == False:
        x = x % tau()
    ret = p = 1
    for i in range(1, nterms):
        ret += (p := -p * x * x / ((2 * i -1) * 2 * i))
    return ret
    return exp(fmod(x, tau()) *1j).real

def tan(x):
    return sin(x) / cos(x)

#loop
def exp_series(x, nterms = 99):
    fact = ret = p = 1
    for n in range(1, nterms):
        ret += (p := p * x) / (fact := fact * n)
    return ret

#2.7182818284590452353602874713527...
def e():
    return exp_series(1)

def exp(x):
    return exp_series(x)

#loop
def sinh(x, nterms = 20):
    ret = p = x
    for i in range(1, nterms):
        ret += (p := p * x * x / ((2 * i + 1) * 2 * i))
    return ret
    foo = exp(x)
    return (foo - 1/foo) / 2

#loop
def cosh(x, nterms = 20):
    ret = p = 1
    for i in range(1, nterms):
        ret += (p := p * x * x / ((2 * i -1) * 2 * i))
    return ret
    foo = exp(x)
    return (foo + 1/foo) / 2

def tanh(x):
    return sinh(x)/cosh(x)
    foo = exp(x+x)
    return (foo - 1) / (foo + 1)
    return tan(1j * x).imag

def expm1(x):
    return (2*tanh(x/2))/(1-tanh(x/2))

def coth(x):
    return 1/tanh(x)

def xpow(x, xp):
    return exp(xp * log(x)) if type(xp) == float else xpow_int(xp, xp)

def fmod(a, b):
    return a % b

def sech(x):
    foo = exp(x)
    return 2 / (foo + 1/foo)

def csch(x):
    foo = exp(x)
    return 2 / (foo - 1/foo)

def int32(n):
    return int(n) & 0xffffffff

def exp2(xp):
    return exp2_int(xp) if type(xp) == int else xpow(2, xp)

#Babylonian method
#loop
def sqrt(num):
    ret = 1
    for term in range(10):
        ret = (ret + num / ret) / 2
    return ret
    return exp(log(x) / 2) if x > 0 else 0

def asinh(x):
    return math.log(x + sqrt(x*x+1))

#loop
def fibonacci(xmax, term1 = 1, term2 = 2):
    yield term1
    yield term2
    while term1 + term2 <= xmax:
        yield term1 + term2
        term1, term2 = term2, term1 + term2

#loop
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
    return (pi() - 2 * asin(x))/2

def sec(x):
    return 1 / cos(x)

def csc(x):
    return 1 / sin(x)

def cot(x):
    return 1 / tan(x)

def isnan(x):
    return numpy.isnan(x)

#loop
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
    if isnan(a) or isnan(b):
        error = 0
    pct = error * 100
    color = 92 if error > 0.99 else 91
    print(f"\033[{color}m{capt} {c} {a} {b} {pct}%\033[0m")

def test1():
    for x in [-10, -5, -0.5, 0, 0.5, 5, 10]:
        test("asinh", x, math.asinh(x), asinh(x))
        test("cos", x, numpy.cos(x), cos(x))
        test("exp", x, numpy.exp(x), exp(x))
        test("expm1", x, math.expm1(x), expm1(x))
        test("sin", x, math.sin(x), sin(x))
        test("tan", x, math.tan(x), tan(x))     # x != pi/2
        test("tanh", x, math.tanh(x), tanh(x))
    for x in [-10, -5, -0.5, 0.5, 5, 10]:
        test("cot", x, 1/math.tan(x), cot(x))
        test("coth", x, 1/tanh(x), coth(x))     # x != 0
    for x in [-0.999, -0.5, 0, 0.5, 0.999]:
        test("acos", x, math.acos(x), acos(x))  # -1 <= x <= 1
        test("asin", x, math.asin(x), asin(x))  # -1 <= x <= 1
        test("atan", x, math.atan(x), atan(x))
        test("atanh", x, math.atanh(x), atanh(x))
        test("atanh_", x, math.atanh(x), atanh_(x))
    for x in [0.5, 1, 2, 20]:
        test("atan", x, math.atan(x), atan(x))
        test("log", x, math.log(x), log(x))
        test("log1p", x, math.log1p(x), log1p(x))
    for x in [0, 0.5, 1, 2, 20]:
        test("sqrt", x, math.sqrt(x), sqrt(x))  # x >= 0
    for x in [-1j, 0.5j, 1j, 0.5+0.5j, 10+1j]:
        test("atanh", x, numpy.arctanh(x), atanh(x))
        test("cos", x, numpy.cos(x), cos(x))
        test("cosh", x, numpy.cosh(x), cosh(x))
        test("csch", x, 1/numpy.sinh(x), csch(x))
        test("exp", x, numpy.exp(x), exp(x))
        test("log1p", x, numpy.log1p(x), log1p(x))
        test("sech", x, 1/numpy.cosh(x), sech(x))
        test("sin", x, numpy.sin(x), sin(x))
        test("sinh", x, numpy.sinh(x), sinh(x))
        test("sqrt", x, numpy.sqrt(x), sqrt(x))
        test("tan", x, numpy.tan(x), tan(x))

def test3(c):
    pass
    #test("csc", c, math.csc(c), csc(c))

def main():
    test1()
    test("atanh", 0.9j, numpy.arctanh(10+0.9j), atanh(10+0.9j))
    test("pi", 1, math.pi, pi())
    test("tau", 1, math.tau, tau())
    test("e", 1, math.e, e())
    print(numpy.cos(1j))
    #print(cos(1j))
    #return

    from matplotlib import pyplot
    a = [atanh(x) for x in numpy.arange(-0.999, 0.999,0.01)]
    #a = [sin_cordic(x) for x in numpy.arange(-10, 10, 0.1)]
    b = [math.atanh(x) for x in numpy.arange(-0.999,0.999,0.01)]
    #b = [abssin(x) for x in numpy.arange(-10,10,0.1)]
    #c = [acos(x) for x in numpy.arange(-0.999,0.999,0.01)]
    pyplot.plot(b)
    pyplot.plot(a)

    #pyplot.plot(c)
    pyplot.show()
    return
    del pyplot
    from matplotlib import pyplot
    a = [atanh(x) for x in numpy.arange(-0.999, 0.999,0.01)]
    #a = [sin_cordic(x) for x in numpy.arange(-10, 10, 0.1)]
    b = [math.atanh(x) for x in numpy.arange(-0.999,0.999,0.01)]
    #b = [abssin(x) for x in numpy.arange(-10,10,0.1)]
    #c = [acos(x) for x in numpy.arange(-0.999,0.999,0.01)]
    pyplot.plot(b)
    pyplot.plot(a)

    #pyplot.plot(c)
    pyplot.show()

if __name__ == "__main__":
    main()



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
    return binary(x, 2)



"""
    rng = mersenne(124)
    for _ in range(4):
        n = next(rng)
        test3(n)
        #test(n, math.exp(n), exp(n))
"""

