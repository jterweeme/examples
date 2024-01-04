#!/usr/bin/pypy3

#Usage ./zcat.py archive.Z > archive

import sys

class BitInputStream:
    def __init__(self, file):
        self.f = open(file, "rb")
        self.bits = self.window = 0
    def readBits(self, n):
        while self.bits < n:
            b = self.f.read(1)
            if len(b) == 0:
                return -1
            c = int.from_bytes(b, 'little')
            self.window |= c << self.bits
            self.bits += 8
        ret = self.window & (1 << n) - 1
        self.window >>= n
        self.bits -= n
        return ret

def codes(bis, bitdepth):
    cnt = 0
    nbits = 9
    while (c := bis.readBits(nbits)) != -1:
        cnt += 1
        if cnt == 1 << nbits - 1 and nbits != bitdepth:
            nbits += 1
            cnt = 0
        if c == 256:
            while cnt % 8 != 0:
                bis.readBits(nbits)
                cnt += 1
            nbits = 9
            cnt = 0
        yield c

def lzw(codegen, bitdepth):
    oldcode = finchar = 0
    xdict = list()
    for newcode in codegen:
        c = newcode
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
        stack.append(finchar := c)
        stack.reverse()
        if len(xdict) + 256 < 1 << bitdepth:
            xdict.append((oldcode, finchar))
        oldcode = newcode
        yield stack

if __name__ == "__main__":
    bis = BitInputStream(sys.argv[1])
    assert bis.readBits(16) == 0x9d1f
    bitdepth = bis.readBits(7)
    assert bitdepth >= 9 and bitdepth <= 16
    assert bis.readBits(1) == 1 #block mode bit is hardcoded in ncompress
    for x in lzw(codes(bis, bitdepth), bitdepth):
        sys.stdout.buffer.write(x)

