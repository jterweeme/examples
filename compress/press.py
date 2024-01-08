#!/usr/bin/pypy3

import sys

if __name__ == "__main__":
    sys.stdout.buffer.write(b'\x1f\x9d\x90')
    n = 0
    nbits = 9
    cnt = 0
    stack = list()
    for line in sys.stdin:
        c = int(line)
        stack.append(c)
        cnt += 1
        if cnt % 8 == 0 or c == 256:
            stack.reverse()
            for x in stack:
                n = n << nbits | x
            stack.clear()
            sys.stdout.buffer.write(n.to_bytes(nbits, 'little'))
            n = 0
        if c == 256:
            nbits = 9
            cnt = 0
        if cnt == 1 << nbits - 1 and nbits != 16:
            nbits += 1
            cnt = 0
    stack.reverse()
    for x in stack:
        n = n << nbits | x
    x = cnt % 8
    x *= nbits
    sys.stdout.buffer.write(n.to_bytes(x // 8 + 1, 'little'))


