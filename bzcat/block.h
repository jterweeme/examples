#ifndef BLOCK_H
#define BLOCK_H

#include "bitstream.h"
#include "table.h"

class Block
{
private:
    int32_t *_merged;
    int32_t _curTbl, _grpIdx, _grpPos, _last, _acc, _repeat, _curp, _length, _dec;
    uint32_t _nextByte();
    uint32_t _nextSymbol(BitInputBase *bi, const Tables &t, const Fugt &selectors);
public:
    void reset();
    Block() { reset(); }
    ~Block() { delete[] _merged; }
    int read();
    void init(BitInputBase *bi);
};

#endif

