/* 
 * Simple DEFLATE decompressor (C++)
 * 
 * Copyright (c) Project Nayuki
 * MIT License. See readme file.
 * https://www.nayuki.io/page/simple-deflate-decompressor
 */

// adapted by Jasper ter Weeme

#include "mystd.h"
#include <bitset>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cassert>

using mystd::ifstream;
using mystd::istream;
using mystd::ostream;
using mystd::string;
using mystd::fill;
using mystd::cout;
using mystd::cin;
using mystd::cerr;
using std::streambuf;

class Toolbox
{
public:
    static char nibble(uint8_t n)
    {
        return n <= 9 ? '0' + char(n) : 'a' + char(n - 10);
    }
    
    static string hex32(uint32_t dw)
    {
        string ret;
        for (uint32_t i = 0; i <= 28; i += 4)
            ret.push_back(nibble(dw >> 28 - i & 0xf));
        return ret;
    }
};

class BitInputStream
{
    istream &_is;
    uint32_t _bits = 0, _window = 0;
public:
    BitInputStream(istream &is) : _is(is) { }

    uint32_t readBits(uint8_t n)
    {
        for (; _bits < n; _bits += 8)
            _window = _window | _is.get() << _bits;

        uint32_t ret = _window & (1 << n) - 1;
        _window = _window >> n, _bits -= n;
        return ret;
    }

    void align() { readBits(_bits); }

    string readNullTerminatedString()
    {
        string ret;
        for (char c; (c = readBits(8)) != 0;)
            ret.push_back(c);
        return ret;
    }
};

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
    ostream &_os;
    CRC32 _crc;
    uint32_t _cnt = 0;
public:
    CRCOutputStream(ostream &os) : _os(os) { }
    uint32_t crc() const { return _crc.crc(); }
    uint32_t cnt() const { return _cnt; }
    void put(uint8_t b) { _os.put(b); ++_cnt; _crc.update(b); }
};

class CanonicalCode final
{
    uint32_t *_symbolCodeBits = nullptr, *_symbolValues = nullptr;
    uint32_t _numSymbolsAllocated = 0;
public:
    void init(uint32_t *codeLengths, uint32_t n);
    uint32_t decodeNextSymbol(BitInputStream &in) const;

    ~CanonicalCode()
    {
        if (_symbolCodeBits)
            delete[] _symbolCodeBits;
    
        if (_symbolValues)
            delete[] _symbolValues;
    }
};

void CanonicalCode::init(uint32_t *codeLengths, uint32_t n)
{
    _symbolCodeBits = new uint32_t[n];
    _symbolValues = new uint32_t[n];

    for (uint32_t codeLength = 1, nextCode = 0; codeLength <= 15; ++codeLength)
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

uint32_t CanonicalCode::decodeNextSymbol(BitInputStream &in) const
{
    for (uint32_t code = 1; true;)
    {
        code = code << 1 | in.readBits(1);
        auto x = std::lower_bound(_symbolCodeBits, _symbolCodeBits + _numSymbolsAllocated, code);

        if (*x == code)
            return _symbolValues[std::distance(_symbolCodeBits, x)];
    }
}

class ByteHistory
{
    uint8_t *_data;
    size_t _index = 0, _length = 0, _size;
public:
    ByteHistory(size_t size) : _size(size) { _data = new uint8_t[size]; }
    ~ByteHistory() { delete[] _data; }

    void append(uint8_t b)
    {
        _data[_index] = b;
        _index = (_index + 1) % _size;
        if (_length < _size)
            ++_length;
    }

    void copy(int dist, int len, CRCOutputStream &out)
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
};

class Inflater
{
    BitInputStream _bis;
    CRCOutputStream _os;
    ostream &_msg;
    ByteHistory _dictionary;
    CanonicalCode _fixedLiteralLengthCode;
    CanonicalCode _fixedDistanceCode;
    void decodeHuffmanCodes(CanonicalCode &litLenCode, CanonicalCode &distCode);
    void inflateUncompressedBlock();
    void inflateHuffmanBlock(const CanonicalCode &litLenCode, const CanonicalCode &distCode);
public:
    Inflater(istream &is, ostream &os, ostream &msg);
    void inflate();
};

Inflater::Inflater(istream &is, ostream &os, ostream &msg)
  :
    _bis(is), _os(os), _msg(msg), _dictionary(32 * 1024)
{
    uint32_t llcodelens[288], distcodelens[32];
    fill(llcodelens,       llcodelens + 144, 8);
    fill(llcodelens + 144, llcodelens + 256, 9);
    fill(llcodelens + 256, llcodelens + 280, 7);
    fill(llcodelens + 280, llcodelens + 288, 8);
    fill(distcodelens, distcodelens + 32, 5);
    _fixedLiteralLengthCode.init(llcodelens, 288);
    _fixedDistanceCode.init(distcodelens, 32);
}

void Inflater::decodeHuffmanCodes(CanonicalCode &litLenCode, CanonicalCode &distCode)
{
    const uint32_t numLitLenCodes = _bis.readBits(5) + 257;  // hlit + 257
    const uint8_t numDistCodes = _bis.readBits(5) + 1;      // hdist + 1
    const uint8_t numCodeLenCodes = _bis.readBits(4) + 4;   // hclen + 4
    uint32_t codeLenCodeLen[19] = {0};
    codeLenCodeLen[16] = _bis.readBits(3);
    codeLenCodeLen[17] = _bis.readBits(3);
    codeLenCodeLen[18] = _bis.readBits(3);
    codeLenCodeLen[ 0] = _bis.readBits(3);

    for (uint32_t i = 0; i < numCodeLenCodes - 4U; ++i)
        codeLenCodeLen[i % 2 == 0 ? 8 + i / 2 : 7 - i / 2] = _bis.readBits(3);

    CanonicalCode codeLenCode;
    codeLenCode.init(codeLenCodeLen, 19);
    const auto nCodeLens = numLitLenCodes + numDistCodes;
    int codeLens[nCodeLens];

    for (auto i = 0u; i < nCodeLens;)
    {
        uint32_t sym = codeLenCode.decodeNextSymbol(_bis);
        if (0 <= sym && sym <= 15)
        {
            codeLens[i++] = sym;
            continue;
        }
        int runLen, runVal = 0;
        if (sym == 16)
            runLen = _bis.readBits(2) + 3, runVal = codeLens[i - 1];
        else if (sym == 17)
            runLen = _bis.readBits(3) + 3;
        else if (sym == 18)
            runLen = _bis.readBits(7) + 11;
        else
            throw std::logic_error("Symbol out of range");

        fill(codeLens + i, codeLens + i + runLen, runVal);
        i += runLen;
    }

    uint32_t litLenCodeLen[numLitLenCodes];
    std::copy(codeLens, codeLens + numLitLenCodes, litLenCodeLen);
    litLenCode.init(litLenCodeLen, numLitLenCodes);
    auto nDistCodeLen = nCodeLens - numLitLenCodes;
    uint32_t distCodeLen[nDistCodeLen];
#if 0
    std::copy(codeLens + numLitLenCodes, codeLens + numLitLenCodes + nCodeLens, distCodeLen);
#else
    for (uint32_t i = 0, j = numLitLenCodes; j < nCodeLens; ++i, ++j)
        distCodeLen[i] = codeLens[j];
#endif
    if (nDistCodeLen == 1 && distCodeLen[0] == 0)
        return;

    int oneCount = 0, otherPositiveCount = 0;
    for (int x : distCodeLen)
    {
        if (x == 1)
            ++oneCount;
        else if (x > 1)
            ++otherPositiveCount;
    }
    
    if (oneCount == 1 && otherPositiveCount == 0)
        nDistCodeLen = 32, distCodeLen[31] = 1;

    distCode.init(distCodeLen, nDistCodeLen);
}

void Inflater::inflateUncompressedBlock()
{
    _bis.align();
    const uint16_t len = _bis.readBits(16);
    const uint16_t nlen = _bis.readBits(16);
    assert(len ^ 0xffff == nlen);
    
    // Copy bytes
    for (uint16_t i = 0; i < len; ++i)
    {
        uint8_t b = _bis.readBits(8);  // Byte is aligned
        _os.put(b);
        _dictionary.append(b);
    }
}

void Inflater::inflateHuffmanBlock(const CanonicalCode &litLenCode, const CanonicalCode &distCode)
{
    for (uint32_t sym; (sym = litLenCode.decodeNextSymbol(_bis)) != 256;)
    {
        if (sym < 256)
        {
            _os.put(sym);
            _dictionary.append(sym);
            continue;
        }
        
        int run, dist;

        if (sym <= 264)
            run = sym - 254;
        else if (sym <= 284)
        {
            auto nExtraBits = (sym - 261U) / 4U;
            run = ((sym - 265) % 4 + 4 << nExtraBits) + 3 + _bis.readBits(nExtraBits);
        }
        else if (sym == 285)
            run = 258;
        else
            throw std::domain_error("Reserved length symbol");

        auto distSym = distCode.decodeNextSymbol(_bis);

        if (distSym <= 3)
            dist = distSym + 1;
        else if (distSym <= 29)
        {
            auto nExtraBits = distSym / 2 - 1;
            dist = (distSym % 2 + 2 << nExtraBits) + 1 + _bis.readBits(nExtraBits);
        }
        else
            throw std::domain_error("Reserved distance symbol");

        _dictionary.copy(dist, run, _os);
    }
}

void Inflater::inflate()
{
    assert(_bis.readBits(16) == 0x8b1f);
    assert(_bis.readBits(8) == 8);  //only support method 8
    std::bitset<8> flags = _bis.readBits(8);
    uint32_t mtime = _bis.readBits(32);

    if (mtime != 0)
        _msg << "Last modified: " << mtime << " (Unix time)\r\n";
    else
        _msg << "Last modified: N/A";
        
    _bis.readBits(16);
        
    if (flags[2])
    {
        _msg << "Flag: Extra\r\n";
        const uint16_t len = _bis.readBits(16);

        for (uint16_t i = 0; i < len; ++i)
            _bis.readBits(8);
    }

    if (flags[3])
        _msg << "File name: " << _bis.readNullTerminatedString() << "\r\n";

    if (flags[4])
        _msg << "Comment: " << _bis.readNullTerminatedString() << "\r\n";

    if (flags[1])
    {
        _bis.readBits(16);
        _msg << "16bit CRC present\r\n";
    }

    for (bool isFinal = false; !isFinal;)
    {
        isFinal = _bis.readBits(1) != 0;

        switch (_bis.readBits(2))
        {
        case 0:
            inflateUncompressedBlock();
            break;
        case 1:
            inflateHuffmanBlock(_fixedLiteralLengthCode, _fixedDistanceCode);
            break;
        case 2:
        {
            CanonicalCode litLen, dist;
            decodeHuffmanCodes(litLen, dist);
            inflateHuffmanBlock(litLen, dist);
        }
            break;
        case 3:
            throw std::domain_error("Reserved block type");
        default:
            throw std::logic_error("Unreachable value");
        }
    }

    _bis.align();
    uint32_t crc = _bis.readBits(32);
    uint32_t size = _bis.readBits(32);
    assert(crc == _os.crc());
    assert(size == _os.cnt());
    _msg << "CRC: 0x" << Toolbox::hex32(crc) << " 0x" << Toolbox::hex32(_os.crc()) << "\r\n";
    _msg << "size: " << size << " " << _os.cnt() << "\r\n";
}

class NullStream : public mystd::ostream {
    public: ostream& operator<<(const char *) override { return *this; }
};

class NullBuf : public streambuf { public: int overflow(int c) override { return c; } };

int main(int argc, char **argv)
{
    static constexpr bool quiet = false;
    istream *is = &cin;
    ostream *msg = &cerr;
    ifstream ifs;
    
    if (argc == 2)
    {
        ifs.open(argv[1]);
        is = &ifs;
    }

#if 0
    NullBuf nb;
    ostream nullStream(&nb);
#else
    NullStream nullStream;
#endif
    if (quiet)
        msg = &nullStream;

    Inflater inflater(*is, cout, *msg);
    inflater.inflate();
    return 0;
}


