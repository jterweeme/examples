#include "block.h"

uint32_t Block::_nextSymbol(BitInputBase *bi, const Tables &t, const Fugt &selectors)
{
    if (++_grpPos % 50 == 0)
        _curTbl = selectors.at(++_grpIdx);

    Table table = t.at(_curTbl);
    uint32_t n = table.minLength();
    int32_t codeBits = bi->readBits(n);

    for (; n <= 23; n++)
    {
        if (codeBits <= table.limit(n))
            return table.symbol(codeBits - table.base(n));

        codeBits = codeBits << 1 | bi->readBits(1);
    }

    return 0;
}

uint32_t Block::_nextByte()
{
    int r = _curp & 0xff;
    _curp = _merged[_curp >> 8];
    _dec++;
    return r;
}

int Block::read()
{
    for (int nextByte; _repeat < 1;)
    {
        if (_dec == _length)
            return -1;

        if ((nextByte = _nextByte()) != _last)
            _last = nextByte, _repeat = 1, _acc = 1;
        else if (++_acc == 4)
            _repeat = _nextByte() + 1, _acc = 0;
        else
            _repeat = 1;
    }

    _repeat--;
    return _last;
}

void Block::reset()
{
    _grpIdx = _grpPos = _last = -1;
    _acc = _repeat = _length = _curp = _dec = _curTbl = 0;
}

void Block::init(BitInputBase *bi)
{
    reset();
    int32_t _bwtByteCounts[256] = {0};
    uint8_t _symbolMap[256] = {0};
    Fugt _bwtBlock2(9000000);
    bi->readInt();
    bi->readBool();
    uint32_t bwtStartPointer = bi->readBits(24), symbolCount = 0;

    for (uint16_t i = 0, ranges = bi->readBits(16); i < 16; i++)
        if ((ranges & (1 << 15 >> i)) != 0)
            for (int j = 0, k = i << 4; j < 16; j++, k++)
                if (bi->readBool())
                    _symbolMap[symbolCount++] = (uint8_t)k;

    uint32_t eob = symbolCount + 1, tables = bi->readBits(3), selectors_n = bi->readBits(15);
    MoveToFront tableMTF2;
    Fugt selectors2(selectors_n);

    for (uint32_t i = 0; i < selectors_n; i++)
    {
        uint32_t x = bi->readUnary();
        uint8_t y = tableMTF2.indexToFront(x);
        selectors2.set(i, y);
    }

    Tables tables2;

    for (uint32_t t = 0; t < tables; t++)
    {
        Table table(symbolCount);

        for (uint32_t i = 0, c = bi->readBits(5); i <= eob; i++)
        {
            while (bi->readBool()) c += bi->readBool() ? -1 : 1;
            table.add(c);
        }

        table.calc();
        tables2.push_back(table);
    }

    //cerr << tables2.toString() << "\n";
    _curTbl = selectors2.at(0);
    MoveToFront symbolMTF2;
    _length = 0;

    for (int n = 0, inc = 1, mtfValue = 0;;)
    {
        uint32_t nextSymbol = _nextSymbol(bi, tables2, selectors2);

        if (nextSymbol == 0)
        {
            n += inc;
            inc <<= 1;
        }
        else if (nextSymbol == 1)
        {
            n += inc << 1;
            inc <<= 1;
        }
        else
        {
            if (n > 0)
            {
                uint8_t nextByte = _symbolMap[mtfValue];
                _bwtByteCounts[nextByte] += n;

                while (--n >= 0)
                    _bwtBlock2.set(_length++, nextByte);

                n = 0;
                inc = 1;
            }

            if (nextSymbol == eob)
                break;

            mtfValue = symbolMTF2.indexToFront(nextSymbol - 1);
            uint8_t nextByte = _symbolMap[mtfValue];
            _bwtByteCounts[nextByte]++;
            _bwtBlock2.set(_length++, nextByte);
        }
    }

    _merged = new int32_t[_length]();
    int characterBase[256] = {0};

    for (int i = 0; i < 255; i++)
        characterBase[i + 1] = _bwtByteCounts[i];

    for (int i = 2; i <= 255; i++)
        characterBase[i] += characterBase[i - 1];

    for (int32_t i = 0; i < _length; i++)
    {
        int value = _bwtBlock2.at(i) & 0xff;
        _merged[characterBase[value]++] = (i << 8) + value;
    }

    _curp = _merged[bwtStartPointer];
}

