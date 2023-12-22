#!/usr/bin/pypy3

#Usage ./zcat.py archive.Z > archive

import sys

class BitInputStream:
    def __init__(self, file):
        self.f = open(file, "rb")
        self.bits = self.window = self.cnt = 0
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
        self.cnt += n
        return ret

class LZW:
    def __init__(self, maxbits):
        self.maxbits = maxbits
        self.oldcode = 0
        self.finchar = 0
        self.dict = list()
    def code(self, incode):
        assert incode != 256 and incode <= len(self.dict) + 256
        c = incode;
        stack = bytearray()
        if c == len(self.dict) + 256:
            stack.append(self.finchar)
            c = self.oldcode
        while c >= 256:
            stack.append(self.dict[c - 256][1])
            c = self.dict[c - 256][0]
        self.finchar = c
        if len(self.dict) + 256 < (1 << self.maxbits):
            self.dict.append((self.oldcode, self.finchar))
        stack.append(self.finchar)
        stack.reverse()
        self.oldcode = incode
        return stack

if __name__ == "__main__":
    bis = BitInputStream(sys.argv[1])
    assert bis.readBits(16) == 0x9d1f
    maxbits = bis.readBits(7)
    assert maxbits >= 9 and maxbits <= 16
    assert bis.readBits(1) == 1 #block mode is always 1?
    bis.cnt = 0  #reset counter needed for cumbersome padding formula
    nbits = 9
    cnt = 0
    lzw = LZW(maxbits)
    while (c := bis.readBits(nbits)) != -1:
        cnt += 1
        if cnt == 1 << nbits - 1 and nbits != maxbits:
            nbits += 1
            cnt = 0
        if c == 256:
            #cumbersome padding formula
            #got it only working with maxbits 13, 15 and 16
            assert maxbits == 13 or maxbits == 15 or maxbits == 16
            nb3 = nbits << 3
            while (bis.cnt - 1 + nb3) % nb3 != nb3 - 1:
                bis.readBits(nbits)
            nbits = 9
            cnt = 0
            lzw.dict.clear()
        else:
            sys.stdout.buffer.write(lzw.code(c))

