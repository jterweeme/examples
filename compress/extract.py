#!/usr/bin/pypy3

#Usage ./extract_codes.py archive.Z | ./codesin.py > archive

import sys

if __name__ == "__main__":
    f = open(sys.argv[1], "rb")
    assert f.read(2) == b'\x1f\x9d'
    c = int.from_bytes(f.read(1), 'little')
    assert c != -1 and c & 0x80 == 0x80
    assert (bitdepth := c & 0x7f) in range(9, 17)
    nbits = 9
    ncodes2 = 0
    while (ncodes := len(buf := bytearray(f.read(nbits))) * 8 // nbits) > 0:
        buf.append(0)
        bits = i = 0
        while ncodes > 0:
            window = buf[bits // 8] | buf[bits // 8 + 1] << 8 | buf[bits // 8 + 2] << 16
            code = window >> i * (nbits - 8) % 8 & (1 << nbits) - 1
            print(code)
            if code == 256:
                nbits = 9
                ncodes2 = 0
                break
            i += 1
            ncodes -= 1
            bits += nbits
            ncodes2 += 1
        if ncodes2 == 1 << nbits - 1 and nbits != bitdepth:
            nbits += 1
            ncodes2 = 0

