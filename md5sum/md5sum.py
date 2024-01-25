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
    a = h
    return a

if __name__ == "__main__":
    h = [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476]
    #h = [0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210]
    k = list(int(2**32 * math.fabs(math.sin(i))) for i in range(1, 65))
    s = [7, 12, 17, 22] * 4 + [5, 9, 14, 20] * 4 + [4, 11, 16, 23] * 4 + [6, 10, 15, 21] * 4
    w = [0] * 16
    fp = sys.stdin.buffer
    b = fp.read(55)
    ba = bytearray(b)
    ba.append(0x80)
    ba.append((55 * 8).to_bytes(4, 'little'))
    ba.append((55 >> 29).to_bytes(4, 'little'))

    
        for i in range(16):
            w[i] = int.from_bytes(fp.read(4), "little")
        calc()
    
    for i in range(4):
        h[i] = swap32(h[i])
    print_hash(h)

