#!/usr/bin/pypy3

import sys

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

if __name__ == "__main__":
    lzw = LZW(16)
    for line in sys.stdin:
        sys.stdout.buffer.write(lzw.code(int(line)))


