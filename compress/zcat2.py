#!/usr/bin/pypy3

#Usage ./zcat.py archive.Z > archive

import sys

def codes(f, bitdepth):
    nbits = 9
    ncodes2 = 0
    while (ncodes := len(buf := bytearray(f.read(nbits))) * 8 // nbits) > 0:
        buf.append(0)
        bits = i = 0
        while ncodes > 0:
            window = buf[bits // 8] | buf[bits // 8 + 1] << 8 | buf[bits // 8 + 2] << 16
            code = window >> i * (nbits - 8) % 8 & (1 << nbits) - 1
            ncodes2 += 1
            ncodes -= 1
            i += 1
            bits += nbits
            yield code
            if code == 256:
                nbits = 9
                ncodes2 = 0
                break
        if ncodes2 == 1 << nbits - 1 and nbits != bitdepth:
            nbits += 1
            ncodes2 = 0

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
    f = open(sys.argv[1], "rb")
    assert f.read(2) == b'\x1f\x9d'
    c = int.from_bytes(f.read(1), 'little')
    assert c != -1 and c & 0x80 == 0x80
    bitdepth = c & 0x7f
    assert bitdepth >= 9 and bitdepth <= 16
    for x in lzw(codes(f, bitdepth), bitdepth):
        sys.stdout.buffer.write(x)

