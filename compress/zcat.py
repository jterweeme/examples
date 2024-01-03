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

class LZW:
    def __init__(self, maxbits):
        self.maxbits = maxbits
        self.oldcode = self.finchar = 0
        self.dict = list()
    def code(self, incode):
        c = incode
        assert c <= len(self.dict) + 256
        if c == 256:
            self.dict.clear()
            return bytearray()
        stack = bytearray()
        if c == len(self.dict) + 256:
            stack.append(self.finchar)
            c = self.oldcode
        while c >= 256:
            stack.append(self.dict[c - 256][1])
            c = self.dict[c - 256][0]
        self.finchar = c
        if len(self.dict) + 256 < 1 << self.maxbits:
            self.dict.append((self.oldcode, self.finchar))
        stack.append(self.finchar)
        stack.reverse()
        self.oldcode = incode
        return stack

def codes(bis, maxbits):
    assert maxbits >= 9 and maxbits <= 16
    cnt = 0
    nbits = 9
    while (c := bis.readBits(nbits)) != -1:
        cnt += 1
        if cnt == 1 << nbits - 1 and nbits != maxbits:
            nbits += 1
            cnt = 0
        if c == 256:
            while cnt % 8 != 0:
                bis.readBits(nbits)
                cnt += 1
            nbits = 9
            cnt = 0
        yield c

if __name__ == "__main__":
    bis = BitInputStream(sys.argv[1])
    assert bis.readBits(16) == 0x9d1f
    maxbits = bis.readBits(7)
    assert bis.readBits(1) == 1 #block mode bit is hardcoded in ncompress
    lzw = LZW(maxbits)
    for c in codes(bis, maxbits):
        sys.stdout.buffer.write(lzw.code(c))

