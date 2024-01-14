#!/usr/bin/pypy3

import sys

if __name__ == "__main__":
    out = sys.stdout.buffer
    out.write(b'\x1f\x9d\x90')
    nbits = 9
    cnt = n = 0
    for lin in sys.stdin:
        c = int(lin)
        n = c << nbits * (cnt % 8) | n
        cnt += 1
        if cnt % 8 == 0 or c == 256:
            out.write(n.to_bytes(nbits, 'little'))
            n = 0
        if c == 256:
            nbits = 9
            cnt = 0
        if cnt == 1 << nbits - 1 and nbits != 16:
            nbits += 1
            cnt = 0
    a = divmod(nbits * (cnt % 8), 8)
    out.write(n.to_bytes(a[0] + (1 if a[1] else 0), 'little'))


