#!/usr/bin/pypy3

#Usage ./extract_codes.py archive.Z | ./lzw > archive

import sys

if __name__ == "__main__":
    f = open(sys.argv[1], "rb")
    assert f.read(2) == b'\x1f\x9d'
    c = int.from_bytes(f.read(1), 'little')
    assert c != -1 and c & 0x80 == 0x80
    assert (bitdepth := c & 0x7f) in range(9, 17)
    nbits = 9
    cnt = 0
    while (ncodes := len(buf := f.read(nbits)) * 8 // nbits) > 0:
        n = int.from_bytes(buf, "little")
        for i in range(ncodes):
            code = n & (1 << nbits) - 1
            print(f'{code}')
            n = n >> nbits
            cnt += 1
            if code == 256:
                nbits = 9
                cnt = 0
                break
        if cnt == 1 << nbits - 1 and nbits != bitdepth:
            nbits += 1
            cnt = 0
            

