#!/usr/bin/pypy3

#Usage: ./zcat_min.py archive.Z > archive

import sys

if __name__ == "__main__":
    f = open(sys.argv[1], "rb")
    assert f.read(2) == b'\x1f\x9d'
    c = int.from_bytes(f.read(1), 'little')
    assert c != -1 and c & 0x80 == 0x80
    bitdepth = c & 0x7f
    assert bitdepth >= 9 and bitdepth <= 16
    nbits = 9
    ncodes2 = oldcode = finchar = 0
    xdict = list()
    while (ncodes := len(buf := bytearray(f.read(nbits))) * 8 // nbits) > 0:
        buf.append(0)
        bits = 0
        i = 0
        while ncodes > 0:
            window = buf[bits // 8] | buf[bits // 8 + 1] << 8 | buf[bits // 8 + 2] << 16
            newcode = c = window >> i * (nbits - 8) % 8 & (1 << nbits) - 1
            assert c <= len(xdict) + 256
            if c == 256:
                xdict.clear()
                nbits = 9
                ncodes2 = 0
                break
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
            i += 1
            ncodes -= 1
            bits += nbits
            ncodes2 += 1
            sys.stdout.buffer.write(stack)
        if ncodes2 == 1 << nbits - 1 and nbits != bitdepth:
            nbits += 1
            ncodes2 = 0
            
            


