/* 
 * Simple DEFLATE decompressor (C++)
 * 
 * Copyright (c) Project Nayuki
 * MIT License. See readme file.
 * https://www.nayuki.io/page/simple-deflate-decompressor
 */

// current version by Jasper ter Weeme

#include <bitset>
#include <iostream>
#include <fstream>
#include <cstdint>

class Toolbox
{
public:
    static char nibble(uint8_t n)
    {
        return n <= 9 ? '0' + char(n) : 'a' + char(n - 10);
    }
    
    static std::string hex32(uint32_t dw)
    {
        std::string ret;
        for (uint32_t i = 0; i <= 28; i += 4)
            ret.push_back(nibble(dw >> (28 - i) & 0xf));
        return ret;
    }

    //https://www.geeksforgeeks.org/binary-search/
    template <typename T> static T binarySearch(T *arr, T l, T r, T x)
    {
        while (l <= r)
        {
            int m = l + (r - l) / 2;
     
            // Check if x is present at mid
            if (arr[m] == x)
                return m;
     
            // If x greater, ignore left half
            if (arr[m] < x)
                l = m + 1;
     
            // If x is smaller, ignore right half
            else
                r = m - 1;
        }
     
        // If we reach here, then element was not present
        return -1;
    }
};

class BitInputStream
{
    std::istream &_input;
    uint8_t _currentByte = 0;
    uint8_t _nBitsRemaining = 0;
public:
    BitInputStream(std::istream &in) : _input(in) { }
    uint8_t getBitPosition() const { return (8 - _nBitsRemaining) % 8; }
    uint8_t readBit();
    void align() { while (getBitPosition() != 0) readBit(); }
    uint32_t readUint(uint32_t numBits);
    std::string readNullTerminatedString()
    {
        std::string ret;
        for (char c; (c = readUint(8)) != 0;)
            ret.push_back(c);
        return ret;
    }
};

//read single bit
uint8_t BitInputStream::readBit()
{
    if (_nBitsRemaining == 0)
    {
        _currentByte = _input.get();
        _nBitsRemaining = 8;
    }
    --_nBitsRemaining;
    return (_currentByte >> 7 - _nBitsRemaining) & 1;
}

//read multiple bits
uint32_t BitInputStream::readUint(uint32_t numBits)
{
    uint32_t result = 0;
    for (uint32_t i = 0; i < numBits; ++i)
    {
        uint32_t c = readBit();
        result |= c << i;
    }
    return result;
}

class CRC32
{
    uint32_t _table[256];
    uint32_t _crc = 0xffffffff;
public:
    void update(char c) { _crc = _table[(_crc ^ c) & 0xff] ^ _crc >> 8; }
    uint32_t crc() const { return ~_crc; }

    CRC32()
    {
        for (uint32_t n = 0; n < 256; ++n)
        {
            uint32_t c = n;
            for (uint32_t k = 0; k < 8; ++k)
                c = c & 1 ? 0xedb88320 ^ c >> 1 : c >> 1;
            _table[n] = c;
        }
    }
};

class CRCOutputStream
{
    std::ostream *_os;
    CRC32 _crc;
    uint32_t _cnt = 0;
public:
    CRCOutputStream(std::ostream *os) : _os(os) { }
    uint32_t crc() const { return _crc.crc(); }
    uint32_t cnt() const { return _cnt; }

    void put(uint8_t b)
    {
        _os->put(b);
        ++_cnt;
        _crc.update(b);
    }
};

class CanonicalCode final
{
    int *_symbolCodeBits = nullptr, *_symbolValues = nullptr;
    int _numSymbolsAllocated = 0;
public:
    ~CanonicalCode();
    void init(int *codeLengths, size_t n);
    int decodeNextSymbol(BitInputStream &in) const;
};

CanonicalCode::~CanonicalCode()
{
    if (_symbolCodeBits)
        delete[] _symbolCodeBits;
    
    if (_symbolValues)
        delete[] _symbolValues;
}

void CanonicalCode::init(int *codeLengths, size_t n)
{
    _symbolCodeBits = new int[n];
    _symbolValues = new int[n];

    for (int codeLength = 1, nextCode = 0; codeLength <= 15; ++codeLength)
    {
        nextCode <<= 1;
        uint32_t startBit = 1 << codeLength;
        for (uint32_t symbol = 0; symbol < n; ++symbol)
        {
            if (codeLengths[symbol] != codeLength)
                continue;
            _symbolCodeBits[_numSymbolsAllocated] = startBit | nextCode;
            _symbolValues[_numSymbolsAllocated] = symbol;
            ++_numSymbolsAllocated;
            ++nextCode;
        }
    }
}

int CanonicalCode::decodeNextSymbol(BitInputStream &in) const
{
    for (int codeBits = 1; true;)
    {
        codeBits = codeBits << 1 | in.readUint(1);
        Toolbox t;
        int index = t.binarySearch<int>(_symbolCodeBits, 0, _numSymbolsAllocated - 1, codeBits);
        if (index >= 0)
            return _symbolValues[index];
    }
}

class ByteHistory
{
    uint8_t *_data;
    size_t _index = 0, _length = 0, _size;
public:
    ByteHistory(size_t size) : _size(size) { _data = new uint8_t[size]; }
    ~ByteHistory() { delete[] _data; }
    void append(int b);
    void copy(int dist, int len, CRCOutputStream &out);
};

void ByteHistory::append(int b)
{
    _data[_index] = uint8_t(b);
    _index = (_index + 1) % _size;
    if (_length < _size) ++_length;
}

void ByteHistory::copy(int dist, int len, CRCOutputStream &out)
{
    size_t readIndex = (_index - dist + _size) % _size;
    for (int i = 0; i < len; ++i)
    {
        uint8_t b = _data[readIndex];
        readIndex = (readIndex + 1) % _size;
        out.put(char(b));
        append(b);
    }
}

class Inflater
{
    BitInputStream * const _bis;
    CRCOutputStream * const _os;
    std::ostream &_msg;
    ByteHistory _dictionary;
    CanonicalCode _FIXED_LITERAL_LENGTH_CODE;
    CanonicalCode _FIXED_DISTANCE_CODE;
    void decodeHuffmanCodes(CanonicalCode &litLenCode, CanonicalCode &distCode);
    void inflateUncompressedBlock();
    void inflateHuffmanBlock(const CanonicalCode &litLenCode, const CanonicalCode &distCode);
    int decodeRunLength(int sym);
    long decodeDistance(int sym);
public:
    Inflater(BitInputStream *bis, CRCOutputStream *os, std::ostream &msg);
    void inflate();
    static void inflate(std::istream &is, std::ostream &os, std::ostream &msg);
};

Inflater::Inflater(BitInputStream *bis, CRCOutputStream *os, std::ostream &msg)
  :
    _bis(bis), _os(os), _msg(msg), _dictionary(32 * 1024)
{
    int llcodelens[288];
    int i = 0;
    for (; i < 144; ++i)
        llcodelens[i] = 8;
    for (; i < 256; ++i)
        llcodelens[i] = 9;
    for (; i < 280; ++i)
        llcodelens[i] = 7;
    for (; i < 288; ++i)
        llcodelens[i] = 8;
    _FIXED_LITERAL_LENGTH_CODE.init(llcodelens, 288);
    int distcodelens[32];
    for (i = 0; i < 32; ++i)
        distcodelens[i] = 5;
    _FIXED_DISTANCE_CODE.init(distcodelens, 32);
}

void Inflater::inflate()
{
    if (_bis->readUint(16) != 0x8b1f)
        throw "invalid magic";

    if (_bis->readUint(8) != 8)
        throw "unsupported compression method";

    std::bitset<8> flags = _bis->readUint(8);
    uint32_t mtime = _bis->readUint(32);

    if (mtime != 0)
        _msg << "Last modified: " << mtime << " (Unix time)\r\n";
    else
        _msg << "Last modified: N/A";
        
    _bis->readUint(16);
        
    if (flags[2])
    {
        _msg << "Flag: Extra\r\n";
        uint16_t len = _bis->readUint(16);

        for (uint16_t i = 0; i < len; ++i)
            _bis->readUint(8);
    }

    if (flags[3])
        _msg << "File name: " + _bis->readNullTerminatedString() << "\r\n";

    if (flags[4])
        _msg << "Comment: " + _bis->readNullTerminatedString() << "\r\n";

    if (flags[1])
    {
        _bis->readUint(16);
        _msg << "16bit CRC present\r\n";
    }

    for (bool isFinal = false; !isFinal;)
    {
        isFinal = _bis->readUint(1) != 0;  // bfinal
        int type = _bis->readUint(2);  // btype
        
        if (type == 0)
            inflateUncompressedBlock();
        else if (type == 1)
            inflateHuffmanBlock(_FIXED_LITERAL_LENGTH_CODE, _FIXED_DISTANCE_CODE);
        else if (type == 2)
        {
            CanonicalCode litLen, dist;
            decodeHuffmanCodes(litLen, dist);
            inflateHuffmanBlock(litLen, dist);
        } else if (type == 3)
            throw std::domain_error("Reserved block type");
        else
            throw std::logic_error("Unreachable value");
    }

    _bis->align();
    uint32_t crc = _bis->readUint(32);
    uint32_t size = _bis->readUint(32);
    _msg << "CRC: 0x" << Toolbox::hex32(crc) << " 0x" << Toolbox::hex32(_os->crc()) << "\r\n";
    _msg << "size: " << size << " " << _os->cnt() << "\r\n";
}

void Inflater::decodeHuffmanCodes(CanonicalCode &litLenCode, CanonicalCode &distCode)
{
    uint32_t numLitLenCodes = _bis->readUint(5) + 257;  // hlit + 257
    uint32_t numDistCodes = _bis->readUint(5) + 1;      // hdist + 1
    uint32_t numCodeLenCodes = _bis->readUint(4) + 4;   // hclen + 4
    int codeLenCodeLen[19] = {0};
    codeLenCodeLen[16] = _bis->readUint(3);
    codeLenCodeLen[17] = _bis->readUint(3);
    codeLenCodeLen[18] = _bis->readUint(3);
    codeLenCodeLen[ 0] = _bis->readUint(3);

    for (uint32_t i = 0; i < numCodeLenCodes - 4; ++i)
    {
        uint32_t j = i % 2 == 0 ? 8 + i / 2 : 7 - i / 2;
        codeLenCodeLen[j] = _bis->readUint(3);
    }
    
    CanonicalCode codeLenCode;
    codeLenCode.init(codeLenCodeLen, 19);
    int nCodeLens = numLitLenCodes + numDistCodes;
    int codeLens[nCodeLens];
    for (int codeLensIndex = 0; codeLensIndex < nCodeLens;)
    {
        int sym = codeLenCode.decodeNextSymbol(*_bis);
        if (0 <= sym && sym <= 15)
        {
            codeLens[codeLensIndex] = sym;
            ++codeLensIndex;
            continue;
        }
        int runLen, runVal = 0;
        if (sym == 16)
        {
            runLen = _bis->readUint(2) + 3;
            runVal = codeLens[codeLensIndex - 1];
        } else if (sym == 17) {
            runLen = _bis->readUint(3) + 3;
        } else if (sym == 18) {
            runLen = _bis->readUint(7) + 11;
        } else {
            throw std::logic_error("Symbol out of range");
        }

        int end = codeLensIndex + runLen;
        for (int i = codeLensIndex; i < end; ++i)
            codeLens[i] = runVal;
        codeLensIndex = end;
    }
    
    int litLenCodeLen[numLitLenCodes];
    for (uint32_t i = 0; i < numLitLenCodes; ++i)
        litLenCodeLen[i] = codeLens[i];
    litLenCode.init(litLenCodeLen, numLitLenCodes);
    int nDistCodeLen = nCodeLens - numLitLenCodes;
    int distCodeLen[nDistCodeLen];
    for (int i = 0, j = numLitLenCodes; j < nCodeLens; ++i, ++j)
        distCodeLen[i] = codeLens[j];

    if (nDistCodeLen == 1 && distCodeLen[0] == 0)
        return;
    
    int oneCount = 0, otherPositiveCount = 0;
    for (int x : distCodeLen)
    {
        if (x == 1)
            oneCount++;
        else if (x > 1)
            otherPositiveCount++;
    }
    
    if (oneCount == 1 && otherPositiveCount == 0)
        nDistCodeLen = 32, distCodeLen[31] = 1;

    distCode.init(distCodeLen, nDistCodeLen);
}

void Inflater::inflateUncompressedBlock()
{
    _bis->align();
    const uint16_t len = _bis->readUint(16);
    const uint16_t nlen = _bis->readUint(16);

    if ((len ^ 0xFFFF) != nlen)
        throw std::domain_error("Invalid length in uncompressed block");
    
    // Copy bytes
    for (uint16_t i = 0; i < len; ++i)
    {
        uint8_t b = _bis->readUint(8);  // Byte is aligned
        _os->put(b);
        _dictionary.append(b);
    }
}

void Inflater::inflateHuffmanBlock(
        const CanonicalCode &litLenCode, const CanonicalCode &distCode)
{
    for (int sym; (sym = litLenCode.decodeNextSymbol(*_bis)) != 256;)
    {
        if (sym < 256)
        {
            _os->put(sym);
            _dictionary.append(sym);
        }
        else
        {
            int run = decodeRunLength(sym);
            int distSym = distCode.decodeNextSymbol(*_bis);
            int dist = decodeDistance(distSym);
            _dictionary.copy(dist, run, *_os);
        }
    }
}

int Inflater::decodeRunLength(int sym)
{
    if (sym <= 264)
        return sym - 254;

    if (sym <= 284)
    {
        uint32_t numExtraBits = (sym - 261) / 4;
        return ((sym - 265) % 4 + 4 << numExtraBits) + 3 + _bis->readUint(numExtraBits);
    }

    if (sym == 285)
        return 258;

    throw std::domain_error("Reserved length symbol");
}

long Inflater::decodeDistance(int sym)
{
    if (sym <= 3)
        return sym + 1;

    if (sym <= 29)
    {
        int numExtraBits = sym / 2 - 1;
        return ((sym % 2 + 2L) << numExtraBits) + 1 + _bis->readUint(numExtraBits);
    }

    throw std::domain_error("Reserved distance symbol");
}

int main(int argc, char *argv[])
{
    std::istream *is = &std::cin;
    std::ifstream ifs;
    
    if (argc == 2)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

    BitInputStream bis(*is);
    CRCOutputStream cos(&std::cout);
    Inflater inflater(&bis, &cos, std::cerr);
    inflater.inflate();
    return 0;
}


