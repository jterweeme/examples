#!/usr/bin/pypy3

import sys
import math

def swap32(x):
    return (((x << 24) & 0xFF000000) |
            ((x <<  8) & 0x00FF0000) |
            ((x >>  8) & 0x0000FF00) |
            ((x >> 24) & 0x000000FF))

def print_hash(a):
    print(f'{a[0]:08x}{a[1]:08x}{a[2]:08x}{a[3]:08x}')

def calc():
    #print_hash(h)
    a = h
    for i in range(2):
        if i < 16:
            f = a[1] & a[2] | ~a[1] & a[3]
            g = i
            print(f)
        elif i < 32:
            f = a[3] & a[1] | ~a[3] & a[2]
            g = (5 * i + 1) % 16
        elif i < 48:
            f = a[1] ^ a[2] ^ a[3]
            g = (3 * i + 5) % 16
        else:
            f = a[2] ^ (a[1] | ~a[3])
            g = (7 * i) % 16
        temp = a[3]
        a[3] = a[2]
        a[2] = a[1]
        x = a[0] + f & 2**32 - 1
        x = x + k[i] & 2**32 - 1
        x = x + w[g] & 2**32 - 1
        print(x)
        a[1] += x << s[i] | x >> 32 - s[i]
        a[1] = a[1] & 2**32 - 1
        a[0] = temp
    return a

if __name__ == "__main__":
    h = [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476]
    #h = [0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210]
    k = list(int(2**32 * math.fabs(math.sin(i))) for i in range(1, 65))
    s = [7, 12, 17, 22] * 4 + [5, 9, 14, 20] * 4 + [4, 11, 16, 23] * 4 + [6, 10, 15, 21] * 4
    w = [0] * 16
    fp = sys.stdin.buffer
    #while len(b := f.read(64)) == 64:
    for _ in range(1):
        for i in range(16):
            w[i] = int.from_bytes(fp.read(4), "big")
        calc()
    
    for i in range(4):
        h[i] = swap32(h[i])
    print_hash(h)

