#!/usr/bin/pypy3

#Usage ./zcat.py archive.Z

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
            self.window = self.window | c << self.bits
            self.bits = self.bits + 8
        ret = self.window & (1 << n) - 1
        self.window = self.window >> n
        self.bits = self.bits - n
        self.cnt = self.cnt + n
        return ret

class LZW:
    def __init__(self, maxbits, block_mode, first):
        self.maxbits = maxbits
        self.oldcode = first
        self.n_bits = 9
        self.finchar = first.to_bytes(1, 'little')
        self.dict = list()
        if block_mode:
            self.dict.append((0, 0))
    def code(self, incode):
        c = incode;
        stack = list()
        if c == len(self.dict) + 256:
            stack.append(self.finchar)
            c = self.oldcode
        while c >= 256:
            stack.append(self.dict[c - 256][1])
            c = self.dict[c - 256][0]
        self.finchar = c.to_bytes(1, 'little')
        yield self.finchar
        for x in reversed(stack):
            yield x
        if len(self.dict) + 256 < (1 << self.maxbits):
            self.dict.append((self.oldcode, self.finchar))
        if self.n_bits < self.maxbits and len(self.dict) + 256 > (1 << self.n_bits) - 1:
            self.n_bits = self.n_bits + 1
        self.oldcode = incode

def main(argv):
    bis = BitInputStream(argv[1])
    assert bis.readBits(16) == 0x9d1f
    maxbits = bis.readBits(5)
    assert maxbits <= 16
    bis.readBits(2)
    block_mode = bis.readBits(1)
    bis.cnt = 0
    first = bis.readBits(9)
    assert first >= 0 and first < 256
    sys.stdout.buffer.write(first.to_bytes(1, 'little'))
    lzw = LZW(maxbits, block_mode, first)
    while True:
        c = bis.readBits(lzw.n_bits)
        if c == -1:
            break
        if c == 256 and block_mode:
            assert maxbits == 13 or maxbits == 15 or maxbits == 16
            nb3 = lzw.n_bits << 3
            while (bis.cnt - 1 + nb3) % nb3 != nb3 - 1:
                bis.readBits(lzw.n_bits)
            lzw.n_bits = 9
            lzw.dict.clear()
        else:
            for b in lzw.code(c):
                sys.stdout.buffer.write(b)

if __name__ == "__main__":
    main(sys.argv)           

