#!/usr/bin/pypy3

#Usage ./compress.py < archive > archive.Z

import sys

class Dictionary:
    def __init__(self):
        self.xdict = [0 for x in range(69001)]
        self.free_ent = 257
    def find(self, c, ent):
        hp = c << 8 ^ ent
        disp = len(self.xdict) - hp - 1
        while self.xdict[hp] != 0:
            if self.xdict[hp][0] == (c << 16 | ent):
                return self.xdict[hp][1]
            hp = hp + len(self.xdict) - disp if hp < disp else hp - disp
        self.xdict[hp] = (c << 16 | ent, self.free_ent)
        self.free_ent += 1
        return 0

def codify(f):
    xdict = Dictionary()
    ent = int.from_bytes(f.read(1), 'little')
    nbits = 9
    extcode = 513
    while len(buf := f.read(1)) > 0:
        byte = int.from_bytes(buf, 'little')
        if xdict.free_ent >= extcode and ent < 257:
            nbits += 1
            if nbits > 16:
                yield 256
                xdict = Dictionary()
                nbits = 9
            extcode = 1 << nbits
            if nbits < 16:
                extcode += 1
        x = xdict.find(byte, ent)
        if x == 0:
            yield ent
        ent = x if x else byte
    yield ent

def press(codes, bitdepth, out):
    out.write(b'\x1f\x9d\x90')
    nbits = 9
    cnt = n = 0
    for c in codes:
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

if __name__ == "__main__":
    press(codify(sys.stdin.buffer), 16, sys.stdout.buffer)


