#!/usr/bin/pypy3

import sys

if __name__ == "__main__":
    oldcode = finchar = 0
    xdict = list()
    for line in sys.stdin:
        newcode = c = int(line)
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
        xdict.append((oldcode, finchar))
        oldcode = newcode
        stack.append(finchar)
        stack.reverse()
        sys.stdout.buffer.write(stack)


