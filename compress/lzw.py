#!/usr/bin/pypy3

import sys

if __name__ == "__main__":
    maxbits = 16
    oldcode = finchar = 0
    xdict = list()
    for line in sys.stdin:
        c = int(line)
        assert c <= len(xdict) + 256
        if c == 256:
            xdict.clear()
            continue
        stack = bytearray()
        if c == len(xdict) + 256:
            stack.append(finchar)
            c = oldcode
        while c >= 256:
            stack.append(xdict[c - 256][1])
            c = xdict[c - 256][0]
        finchar = c
        if len(xdict) + 256 < 1 << maxbits:
            xdict.append((oldcode, finchar))
        oldcode = int(line)
        stack.append(finchar)
        stack.reverse()
        sys.stdout.buffer.write(stack)


