/* 
 * Simple DEFLATE decompressor (C++)
 * 
 * Copyright (c) Project Nayuki
 * MIT License. See readme file.
 * https://www.nayuki.io/page/simple-deflate-decompressor
 */

#include <bitset>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <climits>
#include <optional>
#include <unordered_map>

class BitInputStream
{
private:
    std::istream &input;
    int currentByte;
    int numBitsRemaining;
public:
    explicit BitInputStream(std::istream &in);
    int getBitPosition() const;
    int readBitMaybe();
    int readUint(int numBits);
};

class CanonicalCode final
{
private:
    std::unordered_map<long, int> codeBitsToSymbol;
public:
    explicit CanonicalCode(const std::vector<int> &codeLengths);
    int decodeNextSymbol(BitInputStream &in) const;
    static const int MAX_CODE_LENGTH = 15;
};

class ByteHistory final
{
private:
    std::size_t size;
    std::vector<std::uint8_t> data;
    std::size_t index;
public:
    explicit ByteHistory(std::size_t size);
    void append(std::uint8_t b);
    void copy(long dist, int len, std::ostream &out);
};

class Decompressor final
{
private:
    BitInputStream &input;
    std::ostream *_os;
    ByteHistory dictionary;
    static const CanonicalCode FIXED_LITERAL_LENGTH_CODE;
    static std::vector<int> makeFixedLiteralLengthCode();
    static const CanonicalCode FIXED_DISTANCE_CODE;
    static std::vector<int> makeFixedDistanceCode();
    std::pair<CanonicalCode, std::optional<CanonicalCode>> decodeHuffmanCodes();
    void decompressUncompressedBlock();
    void decompressHuffmanBlock(
        const CanonicalCode &litLenCode, const CanonicalCode &distCode);
    
    int decodeRunLength(int sym);
    long decodeDistance(int sym);
public:
    Decompressor(BitInputStream &in, std::ostream *os);
    void decompress();
    static void decompress(BitInputStream &in, std::vector<uint8_t> &out, uint32_t &n_decomp);
};

BitInputStream::BitInputStream(std::istream &in)
  :
    input(in),
    currentByte(0),
    numBitsRemaining(0)
{}

int BitInputStream::getBitPosition() const
{
    if (numBitsRemaining < 0 || numBitsRemaining > 7)
        throw std::logic_error("Unreachable state");

    return (8 - numBitsRemaining) % 8;
}

int BitInputStream::readBitMaybe()
{
    if (currentByte == std::char_traits<char>::eof())
        return -1;

    if (numBitsRemaining == 0)
    {
        currentByte = input.get();

        if (currentByte == std::char_traits<char>::eof())
            return -1;

        if (currentByte < 0 || currentByte > 255)
            throw std::logic_error("Unreachable value");

        numBitsRemaining = 8;
    }

    if (numBitsRemaining <= 0)
        throw std::logic_error("Unreachable state");

    numBitsRemaining--;
    return (currentByte >> (7 - numBitsRemaining)) & 1;
}

int BitInputStream::readUint(int numBits)
{
    if (numBits < 0 || numBits > 15)
        throw std::domain_error("Number of bits out of range");

    int result = 0;

    for (int i = 0; i < numBits; i++)
    {
        int bit = readBitMaybe();

        if (bit == -1)
            throw std::runtime_error("Unexpected end of stream");

        result |= bit << i;
    }
    return result;
}

CanonicalCode::CanonicalCode(const std::vector<int> &codeLengths)
{
    // Check argument values
    if (codeLengths.size() > INT_MAX)
        throw std::domain_error("Too many symbols");

    for (int x : codeLengths)
    {
        if (x < 0)
            throw std::domain_error("Negative code length");

        if (x > MAX_CODE_LENGTH)
            throw std::domain_error("Maximum code length exceeded");
    }
    
    long nextCode = 0;

    for (int codeLength = 1; codeLength <= MAX_CODE_LENGTH; ++codeLength)
    {
        nextCode <<= 1;
        long startBit = 1L << codeLength;

        for (int symbol = 0; symbol < static_cast<int>(codeLengths.size()); symbol++)
        {
            if (codeLengths[symbol] != codeLength)
                continue;

            if (nextCode >= startBit)
            {
                throw std::domain_error(
                        "This canonical code produces an over-full Huffman code tree");
            }
            codeBitsToSymbol[startBit | nextCode] = symbol;
            ++nextCode;
        }
    }

    if (nextCode != 1L << MAX_CODE_LENGTH)
        throw std::domain_error("This canonical code produces an under-full Huffman code tree");
}

int CanonicalCode::decodeNextSymbol(BitInputStream &in) const
{
    long codeBits = 1;  // The start bit
    while (true) {
        codeBits = (codeBits << 1) | in.readUint(1);
        auto it = codeBitsToSymbol.find(codeBits);
        if (it != codeBitsToSymbol.end())
            return it->second;
    }
}

ByteHistory::ByteHistory(size_t sz) : size(sz), index(0)
{
    if (sz < 1)
        throw std::domain_error("Size must be positive");
}

void ByteHistory::append(uint8_t b)
{
    if (data.size() < size)
        data.push_back(0);  // Dummy value

    assert(index < data.size());
    data[index] = b;
    index = (index + 1U) % size;
}

void ByteHistory::copy(long dist, int len, std::ostream &out)
{
    if (len < 0 || dist < 1 || static_cast<unsigned long>(dist) > data.size())
        throw std::domain_error("Invalid length or distance");
    
    size_t readIndex = (0U + size - dist + index) % size;
    for (int i = 0; i < len; i++) {
        uint8_t b = data[readIndex];
        readIndex = (readIndex + 1U) % size;
        out.put(static_cast<char>(b));
        append(b);
    }
}

void Decompressor::decompress(BitInputStream &in, std::vector<uint8_t> &out, uint32_t &n_decomp)
{
    std::stringstream ss;
    Decompressor d(in, &ss);
    d.decompress();

    while (true)
    {
        int b = ss.get();
        if (b == std::char_traits<char>::eof())
            break;
        out.push_back(static_cast<uint8_t>(b));
        ++n_decomp;
    }
}

void Decompressor::decompress()
{
    bool isFinal;
    do {
        // Read the block header
        isFinal = input.readUint(1) != 0;  // bfinal
        int type = input.readUint(2);  // btype
        
        // Decompress rest of block based on the type
        if (type == 0)
            decompressUncompressedBlock();
        else if (type == 1)
            decompressHuffmanBlock(FIXED_LITERAL_LENGTH_CODE, FIXED_DISTANCE_CODE);
        else if (type == 2) {
            std::pair<CanonicalCode,std::optional<CanonicalCode>> litLenAndDist =
                    decodeHuffmanCodes();

            decompressHuffmanBlock(litLenAndDist.first, litLenAndDist.second.value());
        } else if (type == 3)
            throw std::domain_error("Reserved block type");
        else
            throw std::logic_error("Unreachable value");
    } while (!isFinal);
}

Decompressor::Decompressor(BitInputStream &in, std::ostream *os)
  :
    input(in),
    _os(os),
    dictionary(32U * 1024)
{
}

const CanonicalCode Decompressor::FIXED_LITERAL_LENGTH_CODE(makeFixedLiteralLengthCode());

std::vector<int> Decompressor::makeFixedLiteralLengthCode()
{
    std::vector<int> result;
    int i = 0;
    for (; i < 144; i++) result.push_back(8);
    for (; i < 256; i++) result.push_back(9);
    for (; i < 280; i++) result.push_back(7);
    for (; i < 288; i++) result.push_back(8);
    return result;
}

const CanonicalCode Decompressor::FIXED_DISTANCE_CODE(makeFixedDistanceCode());

std::vector<int> Decompressor::makeFixedDistanceCode() {
    return std::vector<int>(32, 5);
}

std::pair<CanonicalCode,std::optional<CanonicalCode>> Decompressor::decodeHuffmanCodes()
{
    int numLitLenCodes = input.readUint(5) + 257;  // hlit + 257
    int numDistCodes = input.readUint(5) + 1;      // hdist + 1
    int numCodeLenCodes = input.readUint(4) + 4;   // hclen + 4
    std::vector<int> codeLenCodeLen(19, 0);  // This array is filled in a strange order
    codeLenCodeLen[16] = input.readUint(3);
    codeLenCodeLen[17] = input.readUint(3);
    codeLenCodeLen[18] = input.readUint(3);
    codeLenCodeLen[ 0] = input.readUint(3);

    for (int i = 0; i < numCodeLenCodes - 4; i++)
    {
        int j = (i % 2 == 0) ? (8 + i / 2) : (7 - i / 2);
        codeLenCodeLen[j] = input.readUint(3);
    }
    
    // Create the code length code
    CanonicalCode codeLenCode(codeLenCodeLen);
    
    // Read the main code lengths and handle runs
    std::vector<int> codeLens;
    while (codeLens.size() < static_cast<unsigned int>(numLitLenCodes + numDistCodes))
    {
        int sym = codeLenCode.decodeNextSymbol(input);
        if (0 <= sym && sym <= 15)
            codeLens.push_back(sym);
        else {
            int runLen;
            int runVal = 0;

            if (sym == 16)
            {
                if (codeLens.empty())
                    throw std::domain_error("No code length value to copy");

                runLen = input.readUint(2) + 3;
                runVal = codeLens.back();
            } else if (sym == 17) {
                runLen = input.readUint(3) + 3;
            } else if (sym == 18) {
                runLen = input.readUint(7) + 11;
            } else {
                throw std::logic_error("Symbol out of range");
            }

            for (int i = 0; i < runLen; i++)
                codeLens.push_back(runVal);
        }
    }
    if (codeLens.size() > static_cast<unsigned int>(numLitLenCodes + numDistCodes))
        throw std::domain_error("Run exceeds number of codes");
    
    // Create literal-length code tree
    std::vector<int> litLenCodeLen(codeLens.begin(), codeLens.begin() + numLitLenCodes);
    if (litLenCodeLen[256] == 0)
        throw std::domain_error("End-of-block symbol has zero code length");
    CanonicalCode litLenCode(litLenCodeLen);
    
    // Create distance code tree with some extra processing
    std::vector<int> distCodeLen(codeLens.begin() + numLitLenCodes, codeLens.end());
    std::optional<CanonicalCode> distCode;
    if (distCodeLen.size() == 1 && distCodeLen[0] == 0)
        distCode = std::nullopt;  // Empty distance code; the block shall be all literal symbols
    else {
        // Get statistics for upcoming logic
        size_t oneCount = 0;
        size_t otherPositiveCount = 0;
        for (int x : distCodeLen) {
            if (x == 1)
                oneCount++;
            else if (x > 1)
                otherPositiveCount++;
        }
        
        // Handle the case where only one distance code is defined
        if (oneCount == 1 && otherPositiveCount == 0) {
            // Add a dummy invalid code to make the Huffman tree complete
            while (distCodeLen.size() < 32)
                distCodeLen.push_back(0);
            distCodeLen[31] = 1;
        }
        distCode = std::optional<CanonicalCode>(distCodeLen);
    }
    
    return std::pair<CanonicalCode,std::optional<CanonicalCode>>(
        litLenCode, std::move(distCode));
}


void Decompressor::decompressUncompressedBlock()
{
    // Discard bits to align to byte boundary
    while (input.getBitPosition() != 0)
        input.readUint(1);
    
    // Read length
    long  len = static_cast<long>(input.readUint(8)) << 8;   len |= input.readUint(8);
    long nlen = static_cast<long>(input.readUint(8)) << 8;  nlen |= input.readUint(8);
    if ((len ^ 0xFFFF) != nlen)
        throw std::domain_error("Invalid length in uncompressed block");
    
    // Copy bytes
    for (long i = 0; i < len; i++)
    {
        int b = input.readUint(8);  // Byte is aligned
        _os->put(static_cast<char>(b));
        dictionary.append(b);
    }
}

void Decompressor::decompressHuffmanBlock(
        const CanonicalCode &litLenCode, const CanonicalCode &distCode)
{
    while (true)
    {
        int sym = litLenCode.decodeNextSymbol(input);

        //end of block
        if (sym == 256)
            break;
        
        //literal byte
        if (sym < 256)
        {
            uint8_t b = static_cast<uint8_t>(sym);
            _os->put(static_cast<char>(b));
            dictionary.append(b);
            continue;
        }

        // Length and distance for copying
        int run = decodeRunLength(sym);

        if (run < 3 || run > 258)
            throw std::logic_error("Invalid run length");

        int distSym = distCode.decodeNextSymbol(input);
        long dist = decodeDistance(distSym);

        if (dist < 1 || dist > 32768)
            throw std::logic_error("Invalid distance");

        dictionary.copy(dist, run, *_os);
    }
}

int Decompressor::decodeRunLength(int sym)
{
    // Symbols outside the range cannot occur in the bit stream;
    // they would indicate that the decompressor is buggy
    assert(257 <= sym && sym <= 287);
    
    if (sym <= 264)
        return sym - 254;

    if (sym <= 284)
    {
        int numExtraBits = (sym - 261) / 4;
        return (((sym - 265) % 4 + 4) << numExtraBits) + 3 + input.readUint(numExtraBits);
    }

    if (sym == 285)
        return 258;

    // sym is 286 or 287
    throw std::domain_error("Reserved length symbol");
}

long Decompressor::decodeDistance(int sym)
{
    assert(0 <= sym && sym <= 31);
    
    if (sym <= 3)
        return sym + 1;

    if (sym <= 29)
    {
        int numExtraBits = sym / 2 - 1;
        return ((sym % 2 + 2L) << numExtraBits) + 1 + input.readUint(numExtraBits);
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

static uint32_t getCrc32(const std::vector<uint8_t> &data) {
    uint32_t crc = ~UINT32_C(0);
    for (uint8_t b : data) {
        crc ^= b;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ ((crc & 1) * UINT32_C(0xEDB88320));
    }
    return ~crc;
}


static std::string toHex(uint32_t val, int digits)
{
    std::ostringstream s;
    s << std::hex << std::setw(digits) << std::setfill('0') << val;
    return s.str();
}

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
    std::vector<uint8_t> decomp;
    uint32_t n_decomp = 0;
    Decompressor::decompress(in2, decomp, n_decomp);
    uint32_t crc = in1.readLittleEndianUint32();
    std::cerr << "CRC: 0x" << toHex(crc, 8) << "\r\n";
    uint32_t size = in1.readLittleEndianUint32();
    std::cerr << "size: " << size << " " << n_decomp << "\r\n";
    
    if (size != static_cast<uint32_t>(decomp.size()))
        throw "size mismatch";

    if (crc != getCrc32(decomp))
        throw "crc32 mismatch";
        
    for (uint8_t b : decomp)
        os.put(b);
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
