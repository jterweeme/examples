#!/usr/bin/pypy3

#Usage: ./zcat_min.py archive.Z > archive

import sys

if __name__ == "__main__":
    f = open(sys.argv[1], "rb")
    assert f.read(2) == b'\x1f\x9d'
    c = int.from_bytes(f.read(1), 'little')
    assert c != -1 and c & 0x80 == 0x80
    maxbits = c & 0x7f
    assert maxbits >= 9 and maxbits <= 16
    nbits = 9
    
    while True:
        buf = f.read(nbits)
        ncodes = len(buf) * 8 / nbits;
        if ncodes <= 0:
            break;
        while ncodes > 0:
            
            ncodes -= 1
            


