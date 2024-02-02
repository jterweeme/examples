#!/usr/bin/pypy3

#Usage: ./flac2wav < song.flac | ffplay -

#https://www.nayuki.io/page/simple-flac-implementation

import struct, sys

class BitInputStream:
    def __init__(self, inp):
        self.inp = inp
        self.bitbuffer = self.bitbufferlen = 0
    
    def align_to_byte(self):
        self.bitbufferlen -= self.bitbufferlen % 8
    
    def read_byte(self):
        if self.bitbufferlen >= 8:
            return self.read_uint(8)
        result = self.inp.read(1)
        if len(result) == 0:
            return -1
        return result[0]
    
    def read_uint(self, n):
        while self.bitbufferlen < n:
            temp = self.inp.read(1)
            assert len(temp) != 0, "EOF"
            self.bitbuffer = (self.bitbuffer << 8) | temp[0]
            self.bitbufferlen += 8
        self.bitbufferlen -= n
        result = (self.bitbuffer >> self.bitbufferlen) & ((1 << n) - 1)
        self.bitbuffer &= (1 << self.bitbufferlen) - 1
        return result
	
    def read_signed_int(self, n):
        temp = self.read_uint(n)
        temp -= (temp >> (n - 1)) << n
        return temp
    
    def read_rice_signed_int(self, param):
        val = 0
        while self.read_uint(1) == 0:
            val += 1
        val = (val << param) | self.read_uint(param)
        return (val >> 1) ^ -(val & 1)
    
    def close(self):
        self.inp.close()
	
    def __enter__(self):
        return self
	
    def __exit__(self, type, value, traceback):
        self.close()

def restore_linear_prediction(result, coefs, shift):
	for i in range(len(coefs), len(result)):
		result[i] += sum((result[i - 1 - j] * c) for (j, c) in enumerate(coefs)) >> shift

def decode_residuals(inp, blocksize, result):
    method = inp.read_uint(2)
    assert method < 2, "Reserved residual coding method"
    parambits = [4, 5][method]
    escapeparam = [0xF, 0x1F][method]
    partitionorder = inp.read_uint(4)
    numpartitions = 1 << partitionorder
    assert blocksize % numpartitions == 0, "Block size not divisible by number of Rice partitions"
    for i in range(numpartitions):
        count = blocksize >> partitionorder
        if i == 0:
            count -= len(result)
        param = inp.read_uint(parambits)
        if param < escapeparam:
            result.extend(inp.read_rice_signed_int(param) for _ in range(count))
        else:
            numbits = inp.read_uint(5)
            result.extend(inp.read_signed_int(numbits) for _ in range(count))

def decode_subframe(inp, blocksize, sampledepth):
    inp.read_uint(1)
    xtype = inp.read_uint(6)
    if (shift := inp.read_uint(1)) == 1:
        while inp.read_uint(1) == 0:
            shift += 1
    sampledepth -= shift
    if xtype == 0:  # Constant coding
        result = [inp.read_signed_int(sampledepth)] * blocksize
    elif xtype == 1:  # Verbatim coding
        result = [inp.read_signed_int(sampledepth) for _ in range(blocksize)]
    elif 8 <= xtype <= 12:
        result = [inp.read_signed_int(sampledepth) for _ in range(xtype - 8)]
        decode_residuals(inp, blocksize, result)
        restore_linear_prediction(result, ((),(1,),(2,-1),(3,-3,1),(4,-6,4,-1),)[xtype - 8], 0)
    elif 32 <= xtype <= 63:
        result = [inp.read_signed_int(sampledepth) for _ in range(xtype - 31)]
        precision = inp.read_uint(4) + 1
        xshift = inp.read_signed_int(5)
        coefs = [inp.read_signed_int(precision) for _ in range(xtype - 31)]
        decode_residuals(inp, blocksize, result)
        restore_linear_prediction(result, coefs, xshift)
    else:
        raise ValueError("Reserved subframe type")
    return [v << shift for v in result]

if __name__ == "__main__":
    inp = BitInputStream(sys.stdin.buffer)
    out = sys.stdout.buffer
    assert inp.read_uint(32) == 0x664C6143, "Invalid magic string"
    samplerate = None
    last = False
    while not last:
        last = inp.read_uint(1) != 0
        xtype = inp.read_uint(7)
        length = inp.read_uint(24)
        if xtype == 0:  # Stream info block
            inp.read_uint(16)
            inp.read_uint(16)
            inp.read_uint(24)
            inp.read_uint(24)
            samplerate = inp.read_uint(20)
            numchannels = inp.read_uint(3) + 1
            sampledepth = inp.read_uint(5) + 1
            numsamples = inp.read_uint(36)
            inp.read_uint(128)
        else:
            for i in range(length):
                inp.read_uint(8)
    assert samplerate != None, "Stream info metadata block absent"
    assert sampledepth % 8 == 0, "Sample depth not supported"
    sampledatalen = numsamples * numchannels * (sampledepth // 8)
    out.write(b"RIFF")
    out.write(struct.pack("<I", sampledatalen + 36))
    out.write(b"WAVE")
    out.write(b"fmt ")
    out.write(struct.pack("<IHHIIHH", 16, 0x0001, numchannels, samplerate,
        samplerate * numchannels * (sampledepth // 8),
        numchannels * (sampledepth // 8), sampledepth))
    out.write(b"data")
    out.write(struct.pack("<I", sampledatalen))
    while (temp := inp.read_byte()) != -1:
        sync = temp << 6 | inp.read_uint(6)
        assert sync == 0x3ffe, "Sync code expected"
        inp.read_uint(1)
        inp.read_uint(1)
        blocksizecode = inp.read_uint(4)
        sampleratecode = inp.read_uint(4)
        chanasgn = inp.read_uint(4)
        inp.read_uint(3)
        inp.read_uint(1)
        temp = inp.read_uint(8)
        while temp >= 0b11000000:
            inp.read_uint(8)
            temp = (temp << 1) & 0xFF
        if blocksizecode == 1:
            blocksize = 192
        elif 2 <= blocksizecode <= 5:
            blocksize = 576 << blocksizecode - 2
        elif blocksizecode == 6:
            blocksize = inp.read_uint(8) + 1
        elif blocksizecode == 7:
            blocksize = inp.read_uint(16) + 1
        elif 8 <= blocksizecode <= 15:
            blocksize = 256 << (blocksizecode - 8)
        if sampleratecode == 12:
            inp.read_uint(8)
        elif sampleratecode in (13, 14):
            inp.read_uint(16)
        inp.read_uint(8)
        if 0 <= chanasgn <= 7:
            samples = [decode_subframe(inp, blocksize, sampledepth) for _ in range(chanasgn + 1)]
        elif 8 <= chanasgn <= 10:
            temp0 = decode_subframe(inp, blocksize, sampledepth + (1 if (chanasgn == 9) else 0))
            temp1 = decode_subframe(inp, blocksize, sampledepth + (0 if (chanasgn == 9) else 1))
            if chanasgn == 8:
                for i in range(blocksize):
                    temp1[i] = temp0[i] - temp1[i]
            elif chanasgn == 9:
                for i in range(blocksize):
                    temp0[i] += temp1[i]
            elif chanasgn == 10:
                for i in range(blocksize):
                    side = temp1[i]
                    right = temp0[i] - (side >> 1)
                    temp1[i] = right
                    temp0[i] = right + side
            samples = [temp0, temp1]
        else:
            raise "Reserved channel assignment"
        inp.align_to_byte()
        inp.read_uint(16)
        numbytes = sampledepth // 8
        addend = 128 if sampledepth == 8 else 0
        for i in range(blocksize):
            for j in range(numchannels):
                out.write(struct.pack("<i", samples[j][i] + addend)[ : numbytes])


