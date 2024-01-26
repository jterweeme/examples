#!/usr/bin/pypy3

import sys
import math

def print_hash(a):
    for i in range(4):
        sys.stdout.write(a[i].to_bytes(4, 'little').hex())
    print()

def calc(h):
    a, b, c, d = h[0], h[1], h[2], h[3]
    for i in range(64):
        if i < 16:
            f, g = b & c | ~b & d, i
        elif i < 32:
            f, g = d & b | ~d & c, (5 * i + 1) % 16
        elif i < 48:
            f, g = b ^ c ^ d, (3 * i + 5) % 16
        else:
            f, g = c ^ (b | ~d), (7 * i) % 16
        x = (a + f + k[i] + w[g]) % 2**32
        temp = d
        d, c = c, b
        b += x << r[i] | x >> 32 - r[i]
        b = b % 2**32
        a = temp
    return [(x + y) % 2**32 for x, y in zip(h, [a, b, c, d])]

if __name__ == "__main__":
    h = [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476]
    k = list(int(2**32 * math.fabs(math.sin(i))) for i in range(1, 65))
    r = [7, 12, 17, 22] * 4 + [5, 9, 14, 20] * 4 + [4, 11, 16, 23] * 4 + [6, 10, 15, 21] * 4
    w = [0] * 16
    fp = sys.stdin.buffer
    sz = 0
    bar = 17
    while bar > 16:
        foo = 0
        for i in range(16):
            sz += (gc := len(buf := fp.read(4)))
            foo += gc
            w[i] = int.from_bytes(buf, 'little')
            if gc < 4:
                bar = i
                break
        if bar > 16:
            h = calc(h)
    w[bar] |= 0x80 << 8 * gc
    for i in range(bar + 1, 16):
        w[i] = 0;
    if foo >= 56:
        h = calc(h)
        w = [0] * 16
    w[14] = sz << 3
    w[15] = sz >> 29
    h = calc(h)
    print_hash(h)

