#!/usr/bin/pypy3

#Usage ./zcat.py < archive.Z > archive

import sys

def extract_block(f, bitdepth):
    nbits = 9
    cnt = 0
    while (ncodes := len(buf := f.read(nbits)) * 8 // nbits) > 0:
        n = int.from_bytes(buf, "little")
        for i in range(ncodes):
            code = n & (1 << nbits) - 1
            yield code
            n = n >> nbits
            cnt += 1
            if code == 256:
                return
        if cnt == 1 << nbits - 1 and nbits != bitdepth:
            nbits += 1
            cnt = 0

def decompress_block(f, codegen):
    oldcode = finchar = 0
    xdict = list()
    for newcode in codegen:
        c = newcode
        assert c <= len(xdict) + 256
        if c == 256:
            return True
        stack = bytearray()
        if c == len(xdict) + 256:
            stack.append(finchar)
            c = oldcode
        while c >= 256:
            stack.append(xdict[c - 256][1])
            c = xdict[c - 256][0]
        stack.append(finchar := c)
        stack.reverse()
        xdict.append((oldcode, finchar))
        oldcode = newcode
        f.buffer.write(stack)
    return False

if __name__ == "__main__":
    #f = open(sys.argv[1], "rb")
    f = sys.stdin.buffer
    assert f.read(2) == b'\x1f\x9d'
    c = int.from_bytes(f.read(1), "little")
    assert c != -1 and c & 0x80 == 0x80
    assert (bitdepth := c & 0x7f) in range(9, 17)
    while decompress_block(sys.stdout, extract_block(f, bitdepth)):
        pass


