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

def factorial(n):
    return prod(range(2, n + 1))

def xpow_int(x, xp):
    ret = prod([x] * abs(xp))
    return ret if xp >= 0 else 1 / ret

def atan(x):
    return numpy.arctanh(x *1j).imag
    #return math.atan(x)

#3.141592653589793238462643383279502884197...
def pi():
    return 4 * (4 * atan(1/5) - atan(1/239))

#6.283185307179586...
def tau():
    return 8 * (4 * atan(1/5) - atan(1/239))

def exp(x):
    return numpy.exp(x)

#2.7182818284590452353602874713527...
def e():
    return exp(1)

def atanh(x):
    return numpy.arctan(x *1j).imag
    #return numpy.log((1+x)/(1-x))/2

def log1p(x):
    return 2 * atanh(x/(2+x))

def log(x):
    return atanh((x * x - 1) / (x * x + 1)) if x < 1 else log1p(x - 1)

def fmod(a, b):
    return a % b

def cosh(x):
    foo = exp(x)
    return (foo + 1/foo) / 2
    #return numpy.cos(1j * x).real

def sinh(x):
    foo = exp(x)
    return (foo - 1/foo) / 2
    #return numpy.sin(1j * x).imag

def sin(x):
    ret = sinh(x.real * 1j).imag
    if isinstance(x, complex):
        ret = ret * cosh(x.imag) + cosh(x.real * 1j).real * sinh(x.imag) *1j
    return ret

def cos(x):
    def sin_(x):
        return exp((x % tau()) *1j).imag
        #return exp(x * 1j).imag
        #return sinh(x *1j).imag
    def cos_(x):
        return exp((x % tau()) *1j).real
        #return exp(x * 1j).real
        #return cosh(x *1j).real
    ret = cos_(x.real)
    return ret * cosh(x.imag) + sin_(x.real) * sinh(x.imag) *1j if isinstance(x, complex) else ret

def tan(x):
    return sin(x) / cos(x)

def tanh(x):
    foo = exp(x+x)
    return (foo - 1) / (foo + 1)
    #return sinh(x)/cosh(x)
    return tan(1j * x).imag

def expm1(x):
    return (2*tanh(x/2))/(1-tanh(x/2))

def coth(x):
    return 1/tanh(x)

def xpow(x, xp):
    return exp(xp * log(x)) if type(xp) == float else xpow_int(xp, xp)

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

def sqrt(x):
    return exp(log(x) / 2) if x > 0 else 0

def asinh(x):
    return numpy.arcsin(x * 1j).imag
    #return math.log(x + sqrt(x*x+1))

def asin(x):
    return numpy.arcsinh(x * 1j).imag   
    #return atan(x/sqrt(1-x*x)) if abs(x) != 1 else sign(x) * pi() / 2

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
    for x in [0.5, 1, 2, 20]:
        test("atan", x, math.atan(x), atan(x))
        test("log", x, math.log(x), log(x))
        test("log1p", x, math.log1p(x), log1p(x))
    for x in [0, 0.5, 1, 2, 20]:
        test("sqrt", x, math.sqrt(x), sqrt(x))  # x >= 0
    for x in [-1j, 0.5j, 1j, 0.5+0.5j, 10+1j]:
        #test("atanh", x, numpy.arctanh(x), atanh(x))
        test("cos", x, numpy.cos(x), cos(x))
        #test("cosh", x, numpy.cosh(x), cosh(x))
        #test("csch", x, 1/numpy.sinh(x), csch(x))
        test("exp", x, numpy.exp(x), exp(x))
        test("expm1", x, numpy.expm1(x), expm1(x))
        #test("log1p", x, numpy.log1p(x), log1p(x))
        #test("sech", x, 1/numpy.cosh(x), sech(x))
        test("sin", x, numpy.sin(x), sin(x))
        #test("sinh", x, numpy.sinh(x), sinh(x))
        #test("sqrt", x, numpy.sqrt(x), sqrt(x))
        #test("tan", x, numpy.tan(x), tan(x))

def test3(c):
    pass
    #test("csc", c, math.csc(c), csc(c))

def main():
    test1()
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


"""
acos: asin + pi
acosh:
asin: asinh || atan + sqrt
asinh: asin
atan: atanh
atanh: atan
cos_: cosh || exp + [tau]
cos: cos_ + cosh + sin_ + sinh
cosh: cos_ || exp
expm1: tanh
log: atanh
log1p: atanh
sqrt: exp + ln
sin_: exp + [tau] || sinh
sin: cos_ + cosh + sin_ + sinh
tan: cos + sin

"""



