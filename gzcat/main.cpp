/* 
 * Simple DEFLATE decompressor (C++)
 * 
 * Copyright (c) Project Nayuki
 * MIT License. See readme file.
 * https://www.nayuki.io/page/simple-deflate-decompressor
 */

#include <bitset>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cassert>
#include <climits>
#include <cstdint>

class BitInputStream
{
private:
    std::istream &_input;
    int _currentByte = 0;
    int _nBitsRemaining = 0;
    int readBitMaybe();
public:
    BitInputStream(std::istream &in) : _input(in) { }
    int getBitPosition() const { return (8 - _nBitsRemaining) % 8; }
    int readUint(int numBits);
};

int BitInputStream::readBitMaybe()
{
    if (_currentByte == std::char_traits<char>::eof())
        return -1;
    if (_nBitsRemaining == 0)
    {
        _currentByte = _input.get();
        if (_currentByte == std::char_traits<char>::eof())
            return -1;
        _nBitsRemaining = 8;
    }
    --_nBitsRemaining;
    return (_currentByte >> (7 - _nBitsRemaining)) & 1;
}

int BitInputStream::readUint(int numBits)
{
    int result = 0;
    for (int i = 0; i < numBits; i++)
        result |= readBitMaybe() << i;
    return result;
}

class CRCOutputStream
{
private:
    std::ostream *_os;
    uint32_t _crc = 0xffffffff;
    uint32_t _cnt = 0;
public:
    CRCOutputStream(std::ostream *os) : _os(os) { }
    void put(uint8_t b);
    uint32_t crc() const { return ~_crc; }
    uint32_t cnt() const { return _cnt; }
};

void CRCOutputStream::put(uint8_t b)
{
    _os->put(b);
    ++_cnt;
    _crc ^= b;
    for (int i = 0; i < 8; ++i)
        _crc = (_crc >> 1) ^ ((_crc & 1) * UINT32_C(0xEDB88320));
}

class CanonicalCode final
{
private:
    int *_symbolCodeBits, *_symbolValues;
    int _numSymbolsAllocated = 0;
    static const int MAX_CODE_LENGTH = 15;
public:
    CanonicalCode() {}
    CanonicalCode(int *codeLengths, size_t n);
    int decodeNextSymbol(BitInputStream &in) const;
};

CanonicalCode::CanonicalCode(int *codeLengths, size_t n)
{
    _symbolCodeBits = new int[n];
    _symbolValues = new int[n];

    for (int codeLength = 1, nextCode = 0; codeLength <= MAX_CODE_LENGTH; ++codeLength)
    {
        nextCode <<= 1;
        int startBit = 1 << codeLength;
        for (int symbol = 0; symbol < n; ++symbol)
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
        codeBits = (codeBits << 1) | in.readUint(1);
        int index = -1;
        
        for (int i = 0; i < _numSymbolsAllocated; ++i)
        {
            if (_symbolCodeBits[i] == codeBits)
            {   index = i;
                break;
            }
        }
        if (index >= 0)
            return _symbolValues[index];
    }
}

class ByteHistory
{
private:
    uint8_t *_data;
    size_t _index = 0, _length = 0, _size;
public:
    ByteHistory(size_t size) : _size(size) { _data = new uint8_t[size]; }
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

class Inflater final
{
private:
    BitInputStream *_bis;
    CRCOutputStream *_os;
    ByteHistory _dictionary;
    CanonicalCode _FIXED_LITERAL_LENGTH_CODE;
    CanonicalCode _FIXED_DISTANCE_CODE;
    void decodeHuffmanCodes(CanonicalCode &litLenCode, CanonicalCode &distCode);
    void decompressUncompressedBlock();
    void decompressHuffmanBlock(const CanonicalCode &litLenCode, const CanonicalCode &distCode);
    int decodeRunLength(int sym);
    long decodeDistance(int sym);
public:
    Inflater(BitInputStream *bis, CRCOutputStream *os);
    void inflate();
};

Inflater::Inflater(BitInputStream *bis, CRCOutputStream *os)
  :
    _bis(bis), _os(os), _dictionary(32 * 1024)
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
    _FIXED_LITERAL_LENGTH_CODE = CanonicalCode(llcodelens, 288);
    int distcodelens[32];
    for (i = 0; i < 32; ++i)
        distcodelens[i] = 5;
    _FIXED_DISTANCE_CODE = CanonicalCode(distcodelens, 32);
}

static std::string toHex(uint32_t val, int digits)
{
    std::ostringstream s;
    s << std::hex << std::setw(digits) << std::setfill('0') << val;
    return s.str();
}

void Inflater::inflate()
{
    for (bool isFinal = false; !isFinal;)
    {
        isFinal = _bis->readUint(1) != 0;  // bfinal
        int type = _bis->readUint(2);  // btype
        
        if (type == 0)
            decompressUncompressedBlock();
        else if (type == 1)
            decompressHuffmanBlock(_FIXED_LITERAL_LENGTH_CODE, _FIXED_DISTANCE_CODE);
        else if (type == 2)
        {
            CanonicalCode litLen, dist;
            decodeHuffmanCodes(litLen, dist);
            decompressHuffmanBlock(litLen, dist);
        } else if (type == 3)
            throw std::domain_error("Reserved block type");
        else
            throw std::logic_error("Unreachable value");
    }
}

void Inflater::decodeHuffmanCodes(CanonicalCode &litLenCode, CanonicalCode &distCode)
{
    int numLitLenCodes = _bis->readUint(5) + 257;  // hlit + 257
    int numDistCodes = _bis->readUint(5) + 1;      // hdist + 1
    int numCodeLenCodes = _bis->readUint(4) + 4;   // hclen + 4
    int codeLenCodeLen[19];
    codeLenCodeLen[16] = _bis->readUint(3);
    codeLenCodeLen[17] = _bis->readUint(3);
    codeLenCodeLen[18] = _bis->readUint(3);
    codeLenCodeLen[ 0] = _bis->readUint(3);

    for (int i = 0; i < numCodeLenCodes - 4; ++i)
    {
        int j = (i % 2 == 0) ? (8 + i / 2) : (7 - i / 2);
        codeLenCodeLen[j] = _bis->readUint(3);
    }
    
    CanonicalCode codeLenCode(codeLenCodeLen, 19);
    int nCodeLens = numLitLenCodes + numDistCodes;
    int codeLens[nCodeLens];
    for (int codeLensIndex = 0; codeLensIndex < nCodeLens;)
    {
        int sym = codeLenCode.decodeNextSymbol(*_bis);
        if (0 <= sym && sym <= 15)
        {
            codeLens[codeLensIndex] = sym;
            ++codeLensIndex;
        }
        else
        {
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
    }
    
    int litLenCodeLen[numLitLenCodes];
    for (int i = 0; i < numLitLenCodes; ++i)
        litLenCodeLen[i] = codeLens[i];
    litLenCode = CanonicalCode(litLenCodeLen, numLitLenCodes);
    int nDistCodeLen = nCodeLens - numLitLenCodes;
    int distCodeLen[nDistCodeLen];
    for (int i = 0, j = numLitLenCodes; j < nCodeLens; ++i, ++j)
        distCodeLen[i] = codeLens[j];

    if (nDistCodeLen == 1 && distCodeLen[0] == 0)
    {
        // Empty distance code; the block shall be all literal symbols
        distCode = CanonicalCode(distCodeLen, nDistCodeLen);
    }
    else
    {
        int oneCount = 0, otherPositiveCount = 0;
        for (int x : distCodeLen)
        {
            if (x == 1)
                oneCount++;
            else if (x > 1)
                otherPositiveCount++;
        }
        
        if (oneCount == 1 && otherPositiveCount == 0) {
            nDistCodeLen = 32;
            distCodeLen[31] = 1;
        }
        distCode = CanonicalCode(distCodeLen, nDistCodeLen);
    }
}

void Inflater::decompressUncompressedBlock()
{
    // Discard bits to align to byte boundary
    while (_bis->getBitPosition() != 0)
        _bis->readUint(1);
    
    // Read length
    int len = _bis->readUint(16);
    int nlen = _bis->readUint(16);

    if ((len ^ 0xFFFF) != nlen)
        throw std::domain_error("Invalid length in uncompressed block");
    
    // Copy bytes
    for (int i = 0; i < len; ++i)
    {
        int b = _bis->readUint(8);  // Byte is aligned
        _os->put(b);
        _dictionary.append(b);
    }
}

void Inflater::decompressHuffmanBlock(
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
        int numExtraBits = (sym - 261) / 4;
        return (((sym - 265) % 4 + 4) << numExtraBits) + 3 + _bis->readUint(numExtraBits);
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

class DataInput final
{
private:
    std::istream &input;
public:
    DataInput(std::istream &in) : input(in) {}
    
    uint8_t readUint8() {
        int b = input.get();
        if (b == std::char_traits<char>::eof())
            throw std::runtime_error("Unexpected end of stream");
        return static_cast<uint8_t>(b);
    }
    
    uint16_t readLittleEndianUint16() {
        uint16_t result = 0;
        for (int i = 0; i < 2; i++)
            result |= static_cast<uint16_t>(readUint8()) << (i * 8);
        return result;
    }
    
    uint32_t readLittleEndianUint32() {
        uint32_t result = 0;
        for (int i = 0; i < 4; i++)
            result |= static_cast<uint32_t>(readUint8()) << (i * 8);
        return result;
    }
    
    std::string readNullTerminatedString()
    {
        std::string result;
        while (true) {
            int b = input.get();
            if (b == std::char_traits<char>::eof())
                throw std::runtime_error("Unexpected end of stream");
            else if (b == '\0')
                break;
            else
                result.push_back(static_cast<char>(b));
        }
        return result;
    }
};

static void submain(std::istream &is, std::ostream &os)
{
    DataInput in1(is);

    if (in1.readLittleEndianUint16() != 0x8B1F)
        throw "invalid magic";

    int compMeth = in1.readUint8();

    if (compMeth != 8)
        throw "unsupported compression method";

    std::bitset<8> flags = in1.readUint8();
    uint32_t mtime = in1.readLittleEndianUint32();

    if (mtime != 0)
        std::cerr << "Last modified: " << mtime << " (Unix time)" << std::endl;
    else
        std::cerr << "Last modified: N/A";
        
    int extraFlags = in1.readUint8();
    in1.readUint8();
        
    if (flags[2])
    {
        std::cerr << "Flag: Extra" << std::endl;
        long len = in1.readLittleEndianUint16();

        for (long i = 0; i < len; i++)
            in1.readUint8();
    }

    if (flags[3])
        std::cerr << "File name: " + in1.readNullTerminatedString() << std::endl;

    if (flags[4])
        std::cerr << "Comment: " + in1.readNullTerminatedString() << std::endl;

    if (flags[1])
    {
        std::cerr << "Header CRC-16: "
                  << toHex(in1.readLittleEndianUint16(), 4) << std::endl;
    }
        
    BitInputStream in2(is);
    CRCOutputStream os2(&os);
    Inflater d(&in2, &os2);
    d.inflate();
    uint32_t crc = in1.readLittleEndianUint32();
    std::cerr << "CRC: 0x" << toHex(crc, 8) << " 0x" << toHex(os2.crc(), 8) << "\r\n";
    uint32_t size = in1.readLittleEndianUint32();
    std::cerr << "size: " << size << " " << os2.cnt() << "\r\n";
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

    submain(*is, std::cout);
    return 0;
}
