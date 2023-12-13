import sys

class BitInputStream:
    def readBits:
        return 0

class LZW:
    def __init__(self, maxbits, block_mode, first):
        self.maxbits = maxbits
        self.block_mode = block_mode
        self.oldcode = first
        self.free_ent = 257 if block_mode else 256
        self.finchar = first
        self.htab = list(range(1 << maxbits))
        self.codetab = list(range(1 << maxbits))
        self.n_bits = 9
    def code(self, incode):
        c = incode;
        stack = list()
        if c == self.free_ent:
            stack.append(self.finchar)
            c = self.oldcode
        while c >= 256:
            stack.append(self.htab[c])
            c = self.codetab[c]
        self.finchar = self.htab[c]
        stack.append(self.finchar)
        stack.reverse()
        if self.free_ent < (1 << self.maxbits):
            self.codetab[self.free_ent] = self.oldcode
            self.htab[self.free_ent] = self.finchar
        maxcode = 1 << self.maxbits if self.n_bits == self.maxbits else (1 << self.n_bits) - 1
        if self.free_ent > maxcode:
            self.n_bits = self.n_bits + 1
        self.oldcode = incode
        return stack

        
            



