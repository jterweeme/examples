#!/usr/bin/pypy3

#Usage ./codesout.py archive.Z | ./codesin.py > archive

import sys

class BitInputStream:
    def __init__(self, file):
        self.f = open(file, "rb")
        self.bits = 0
        self.window = 0
        self.cnt = 0
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

if __name__ == "__main__":
    bis = BitInputStream(sys.argv[1])
    assert bis.readBits(16) == 0x9d1f
    maxbits = bis.readBits(7)
    assert maxbits >= 9 and maxbits <= 16
    assert bis.readBits(1) == 1 #block mode is always 1?
    bis.cnt = 0  #reset counter needed for cumbersome padding formula
    nbits = 9
    cnt = 0
    while (c := bis.readBits(nbits)) != -1:
        cnt += 1
        if cnt == 1 << nbits - 1 and nbits != maxbits:
            nbits += 1
            cnt = 0
        if c == 256:
            #cumbersome padding formula
            assert maxbits == 13 or maxbits == 15 or maxbits == 16
            nb3 = nbits << 3
            while (bis.cnt - 1 + nb3) % nb3 != nb3 - 1:
                bis.readBits(nbits)
            nbits = 9
            cnt = 0
        print(f'{c}')

