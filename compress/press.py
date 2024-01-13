#!/usr/bin/pypy3

import sys

def write_chunk(s, nbits, l, out):
    s.reverse()
    n = 0
    for x in s:
        n = n << nbits | x
    x = divmod(l * nbits, 8)
    out.write(n.to_bytes(x[0] + (1 if x[1] else 0), 'little'))
    s.clear()

if __name__ == "__main__":
    sys.stdout.buffer.write(b'\x1f\x9d\x90')
    nbits = 9
    cnt = 0
    stack = list()
    for lin in sys.stdin:
        c = int(lin)
        stack.append(c)
        cnt += 1
        if len(stack) == 8 or c == 256:
            write_chunk(stack, nbits, 8, sys.stdout.buffer)
        if c == 256:
            nbits = 9
            cnt = 0
        if cnt == 1 << nbits - 1 and nbits != 16:
            nbits += 1
            cnt = 0
    write_chunk(stack, nbits, len(stack), sys.stdout.buffer)


