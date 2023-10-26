/* 
 * Simple DEFLATE decompressor (C++)
 * 
 * Copyright (c) Project Nayuki
 * MIT License. See readme file.
 * https://www.nayuki.io/page/simple-deflate-decompressor
 */

#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <climits>
#include <optional>
#include <unordered_map>


/* 
 * A stream of bits that can be read. Because they come from an underlying byte stream, the
 * total number of bits is always a multiple of 8. Bits are packed in little endian within
 * a byte. For example, the byte 0x87 reads as the sequence of bits [1,1,1,0,0,0,0,1].
 */
class BitInputStream {
    
    /*---- Fields ----*/
    
    // The underlying byte stream to read from.
    private: std::istream &input;
    
    // Either in the range [0x00, 0xFF] if bits are available, or EOF if end of stream is reached.
    private: int currentByte;
    
    // Number of remaining bits in the current byte, in the range [0, 8).
    private: int numBitsRemaining;
    
    
    /*---- Constructor ----*/
    
    // Constructs a bit input stream based on the given byte input stream.
    public: explicit BitInputStream(std::istream &in);
    
    
    /*---- Methods ----*/
    
    // Returns the current bit position, which ascends from 0 to 7 as bits are read.
    public: int getBitPosition() const;
    
    
    // Reads a bit from this stream. Returns 0 or 1 if a bit is available, or -1 if
    // the end of stream is reached. The end of stream always occurs on a byte boundary.
    public: int readBitMaybe();
    
    
    // Reads the given number of bits from this stream,
    // packing them in little endian as an unsigned integer.
    public: int readUint(int numBits);
    
};



/* 
 * A canonical Huffman code, where the code values for each symbol is
 * derived from a given sequence of code lengths. This data structure is
 * immutable. This could be transformed into an explicit Huffman code tree.
 * 
 * Example:
 *   Code lengths (canonical code):
 *     Symbol A: 1
 *     Symbol B: 0 (no code)
 *     Symbol C: 3
 *     Symbol D: 2
 *     Symbol E: 3
 *  
 *   Generated Huffman codes:
 *     Symbol A: 0
 *     Symbol B: (Absent)
 *     Symbol C: 110
 *     Symbol D: 10
 *     Symbol E: 111
 *  
 *   Huffman code tree:
 *       .
 *      / \
 *     A   .
 *        / \
 *       D   .
 *          / \
 *         C   E
 */
class CanonicalCode final
{
private:
    /*---- Field ----*/
    
    // This dictionary maps Huffman codes to symbol values. Each key is the
    // Huffman code padded with a 1 bit at the beginning to disambiguate codes
    // of different lengths (e.g. otherwise we can't distinguish 0b01 from
    // 0b0001). For the example of codeLengths=[1,0,3,2,3], we would have:
    //     0b1_0 -> 0
    //    0b1_10 -> 3
    //   0b1_110 -> 2
    //   0b1_111 -> 4
    std::unordered_map<long,int> codeBitsToSymbol;
    
    
    /*---- Constructor ----*/
    
    // Constructs a canonical Huffman code from the given list of symbol code lengths.
    // Each code length must be non-negative. Code length 0 means no code for the symbol.
    // The collection of code lengths must represent a proper full Huffman code tree.
    // Examples of code lengths that result in correct full Huffman code trees:
    // - [1, 1] (result: A=0, B=1)
    // - [2, 2, 1, 0, 0, 0] (result: A=10, B=11, C=0)
    // - [3, 3, 3, 3, 3, 3, 3, 3] (result: A=000, B=001, C=010, ..., H=111)
    // Examples of code lengths that result in under-full Huffman code trees:
    // - [0, 2, 0] (result: B=00, unused=01, unused=1)
    // - [0, 1, 0, 2] (result: B=0, D=10, unused=11)
    // Examples of code lengths that result in over-full Huffman code trees:
    // - [1, 1, 1] (result: A=0, B=1, C=overflow)
    // - [1, 1, 2, 2, 3, 3, 3, 3] (result: A=0, B=1, C=overflow, ...)
    public: explicit CanonicalCode(const std::vector<int> &codeLengths);
    
    
    /*---- Method ----*/
    
    // Decodes the next symbol from the given bit input stream based on this
    // canonical code. The returned symbol value is in the range [0, codeLengths.size()).
    public: int decodeNextSymbol(BitInputStream &in) const;
    
    
    /*---- Constant ----*/
    
    // The maximum Huffman code length allowed in the DEFLATE standard.
    private: static const int MAX_CODE_LENGTH = 15;
    
};



/* 
 * Stores a finite recent history of a byte stream. Useful as an implicit
 * dictionary for Lempel-Ziv schemes.
 */
class ByteHistory final {
    
    /*---- Fields ----*/
    
    // Maximum number of bytes stored in this history.
    private: std::size_t size;
    
    // Circular buffer of byte data.
    private: std::vector<std::uint8_t> data;
    
    // Index of next byte to write to, always in the range [0, data.size()).
    private: std::size_t index;
    
    
    /*---- Constructor ----*/
    
    // Constructs a byte history of the given size.
    public: explicit ByteHistory(std::size_t size);
    
    
    /*---- Methods ----*/
    
    // Appends the given byte to this history.
    // This overwrites the byte value at `size` positions ago.
    public: void append(std::uint8_t b);
    
    
    // Copies `len` bytes starting at `dist` bytes ago to the
    // given output stream and also back into this buffer itself.
    // Note that if the count exceeds the distance, then some of the output
    // data will be a copy of data that was copied earlier in the process.
    public: void copy(long dist, int len, std::ostream &out);
    
};



/* 
 * Decompresses raw DEFLATE data (without zlib or gzip container) into bytes.
 */
class Decompressor final {
    
    /*---- Public functions ----*/
    
    // Reads from the given input stream, decompresses the data, and returns a new byte array.
    public: static std::vector<std::uint8_t> decompress(BitInputStream &in);
    
    
    // Reads from the given input stream, decompresses the data, and writes to the given output stream.
    public: static void decompress(BitInputStream &in, std::ostream &out);
    
    
    
    /*---- Private implementation ----*/
    
    /*-- Fields --*/
    
    private: BitInputStream &input;
    
    private: std::ostream &output;
    
    private: ByteHistory dictionary;
    
    
    /*-- Constructor --*/
    
    // Constructor, which immediately performs decompression
    private: explicit Decompressor(BitInputStream &in, std::ostream &out);
    
    
    /*-- Constants: The code trees for static Huffman codes (btype = 1) --*/
    
    private: static const CanonicalCode FIXED_LITERAL_LENGTH_CODE;
    private: static std::vector<int> makeFixedLiteralLengthCode();
    
    private: static const CanonicalCode FIXED_DISTANCE_CODE;
    private: static std::vector<int> makeFixedDistanceCode();
    
    
    /*-- Method: Reading and decoding dynamic Huffman codes (btype = 2) --*/
    
    // Reads from the bit input stream, decodes the Huffman code
    // specifications into code trees, and returns the trees.
    private: std::pair<CanonicalCode,std::optional<CanonicalCode>> decodeHuffmanCodes();
    
    
    /*-- Methods: Block decompression --*/
    
    // Handles and copies an uncompressed block from the bit input stream.
    private: void decompressUncompressedBlock();
    
    // Decompresses a Huffman-coded block from the bit input stream based on the given Huffman codes.
    private: void decompressHuffmanBlock(
        const CanonicalCode &litLenCode, const std::optional<CanonicalCode> &distCode);
    
    
    /*-- Methods: Symbol decoding --*/
    
    // Returns the run length based on the given symbol and possibly reading more bits.
    private: int decodeRunLength(int sym);
    
    // Returns the distance based on the given symbol and possibly reading more bits.
    private: long decodeDistance(int sym);
    
};


using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::string;
using std::uint8_t;
using std::size_t;
using std::vector;
using std::domain_error;
using std::logic_error;
using std::runtime_error;

BitInputStream::BitInputStream(std::istream &in) :
    input(in),
    currentByte(0),
    numBitsRemaining(0) {}


int BitInputStream::getBitPosition() const {
    if (numBitsRemaining < 0 || numBitsRemaining > 7)
        throw logic_error("Unreachable state");
    return (8 - numBitsRemaining) % 8;
}


int BitInputStream::readBitMaybe() {
    if (currentByte == std::char_traits<char>::eof())
        return -1;
    if (numBitsRemaining == 0) {
        currentByte = input.get();  // Note: istream.get() returns int, not char
        if (currentByte == std::char_traits<char>::eof())
            return -1;
        if (currentByte < 0 || currentByte > 255)
            throw logic_error("Unreachable value");
        numBitsRemaining = 8;
    }
    if (numBitsRemaining <= 0)
        throw logic_error("Unreachable state");
    numBitsRemaining--;
    return (currentByte >> (7 - numBitsRemaining)) & 1;
}


int BitInputStream::readUint(int numBits) {
    if (numBits < 0 || numBits > 15)
        throw domain_error("Number of bits out of range");
    int result = 0;
    for (int i = 0; i < numBits; i++) {
        int bit = readBitMaybe();
        if (bit == -1)
            throw runtime_error("Unexpected end of stream");
        result |= bit << i;
    }
    return result;
}



/*---- CanonicalCode class ----*/

CanonicalCode::CanonicalCode(const vector<int> &codeLengths) {
    // Check argument values
    if (codeLengths.size() > INT_MAX)
        throw domain_error("Too many symbols");
    for (int x : codeLengths) {
        if (x < 0)
            throw domain_error("Negative code length");
        if (x > MAX_CODE_LENGTH)
            throw domain_error("Maximum code length exceeded");
    }
    
    // Allocate code values to symbols. Symbols are processed in the order
    // of shortest code length first, breaking ties by lowest symbol value.
    long nextCode = 0;
    for (int codeLength = 1; codeLength <= MAX_CODE_LENGTH; codeLength++) {
        nextCode <<= 1;
        long startBit = 1L << codeLength;
        for (int symbol = 0; symbol < static_cast<int>(codeLengths.size()); symbol++) {
            if (codeLengths[symbol] != codeLength)
                continue;
            if (nextCode >= startBit)
                throw domain_error("This canonical code produces an over-full Huffman code tree");
            codeBitsToSymbol[startBit | nextCode] = symbol;
            nextCode++;
        }
    }
    if (nextCode != 1L << MAX_CODE_LENGTH)
        throw domain_error("This canonical code produces an under-full Huffman code tree");
}


int CanonicalCode::decodeNextSymbol(BitInputStream &in) const {
    long codeBits = 1;  // The start bit
    while (true) {
        // Accumulate one bit at a time on the right side until a match is found
        // in the symbolCodeBits array. Because the Huffman code tree is full,
        // this loop must terminate after at most MAX_CODE_LENGTH iterations.
        codeBits = (codeBits << 1) | in.readUint(1);
        auto it = codeBitsToSymbol.find(codeBits);
        if (it != codeBitsToSymbol.end())
            return it->second;
    }
}



/*---- ByteHistory class ----*/

ByteHistory::ByteHistory(size_t sz) :
        size(sz),
        index(0) {
    if (sz < 1)
        throw domain_error("Size must be positive");
}


void ByteHistory::append(uint8_t b) {
    if (data.size() < size)
        data.push_back(0);  // Dummy value
    assert(index < data.size());
    data[index] = b;
    index = (index + 1U) % size;
}


void ByteHistory::copy(long dist, int len, std::ostream &out) {
    if (len < 0 || dist < 1 || static_cast<unsigned long>(dist) > data.size())
        throw domain_error("Invalid length or distance");
    
    size_t readIndex = (0U + size - dist + index) % size;
    for (int i = 0; i < len; i++) {
        uint8_t b = data[readIndex];
        readIndex = (readIndex + 1U) % size;
        out.put(static_cast<char>(b));
        append(b);
    }
}



/*---- Decompressor class ----*/

vector<uint8_t> Decompressor::decompress(BitInputStream &in) {
    std::stringstream ss;
    decompress(in, ss);
    vector<uint8_t> result;
    while (true) {
        int b = ss.get();
        if (b == std::char_traits<char>::eof())
            break;
        result.push_back(static_cast<uint8_t>(b));
    }
    return result;
}


void Decompressor::decompress(BitInputStream &in, std::ostream &out) {
    Decompressor(in, out);
}


Decompressor::Decompressor(BitInputStream &in, std::ostream &out) :
        // Initialize fields
        input(in),
        output(out),
        dictionary(32U * 1024) {
    
    // Process the stream of blocks
    bool isFinal;
    do {
        // Read the block header
        isFinal = in.readUint(1) != 0;  // bfinal
        int type = input.readUint(2);  // btype
        
        // Decompress rest of block based on the type
        if (type == 0)
            decompressUncompressedBlock();
        else if (type == 1)
            decompressHuffmanBlock(FIXED_LITERAL_LENGTH_CODE, FIXED_DISTANCE_CODE);
        else if (type == 2) {
            std::pair<CanonicalCode,std::optional<CanonicalCode>> litLenAndDist = decodeHuffmanCodes();
            decompressHuffmanBlock(litLenAndDist.first, litLenAndDist.second);
        } else if (type == 3)
            throw domain_error("Reserved block type");
        else
            throw logic_error("Unreachable value");
    } while (!isFinal);
}


const CanonicalCode Decompressor::FIXED_LITERAL_LENGTH_CODE(makeFixedLiteralLengthCode());

vector<int> Decompressor::makeFixedLiteralLengthCode() {
    vector<int> result;
    int i = 0;
    for (; i < 144; i++) result.push_back(8);
    for (; i < 256; i++) result.push_back(9);
    for (; i < 280; i++) result.push_back(7);
    for (; i < 288; i++) result.push_back(8);
    return result;
}


const CanonicalCode Decompressor::FIXED_DISTANCE_CODE(makeFixedDistanceCode());

vector<int> Decompressor::makeFixedDistanceCode() {
    return vector<int>(32, 5);
}


std::pair<CanonicalCode,std::optional<CanonicalCode>> Decompressor::decodeHuffmanCodes() {
    int numLitLenCodes = input.readUint(5) + 257;  // hlit + 257
    int numDistCodes = input.readUint(5) + 1;      // hdist + 1
    
    // Read the code length code lengths
    int numCodeLenCodes = input.readUint(4) + 4;   // hclen + 4
    vector<int> codeLenCodeLen(19, 0);  // This array is filled in a strange order
    codeLenCodeLen[16] = input.readUint(3);
    codeLenCodeLen[17] = input.readUint(3);
    codeLenCodeLen[18] = input.readUint(3);
    codeLenCodeLen[ 0] = input.readUint(3);
    for (int i = 0; i < numCodeLenCodes - 4; i++) {
        int j = (i % 2 == 0) ? (8 + i / 2) : (7 - i / 2);
        codeLenCodeLen[j] = input.readUint(3);
    }
    
    // Create the code length code
    CanonicalCode codeLenCode(codeLenCodeLen);
    
    // Read the main code lengths and handle runs
    vector<int> codeLens;
    while (codeLens.size() < static_cast<unsigned int>(numLitLenCodes + numDistCodes)) {
        int sym = codeLenCode.decodeNextSymbol(input);
        if (0 <= sym && sym <= 15)
            codeLens.push_back(sym);
        else {
            int runLen;
            int runVal = 0;
            if (sym == 16) {
                if (codeLens.empty())
                    throw domain_error("No code length value to copy");
                runLen = input.readUint(2) + 3;
                runVal = codeLens.back();
            } else if (sym == 17)
                runLen = input.readUint(3) + 3;
            else if (sym == 18)
                runLen = input.readUint(7) + 11;
            else
                throw logic_error("Symbol out of range");
            for (int i = 0; i < runLen; i++)
                codeLens.push_back(runVal);
        }
    }
    if (codeLens.size() > static_cast<unsigned int>(numLitLenCodes + numDistCodes))
        throw domain_error("Run exceeds number of codes");
    
    // Create literal-length code tree
    vector<int> litLenCodeLen(codeLens.begin(), codeLens.begin() + numLitLenCodes);
    if (litLenCodeLen[256] == 0)
        throw domain_error("End-of-block symbol has zero code length");
    CanonicalCode litLenCode(litLenCodeLen);
    
    // Create distance code tree with some extra processing
    vector<int> distCodeLen(codeLens.begin() + numLitLenCodes, codeLens.end());
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
        std::move(litLenCode), std::move(distCode));
}


void Decompressor::decompressUncompressedBlock() {
    // Discard bits to align to byte boundary
    while (input.getBitPosition() != 0)
        input.readUint(1);
    
    // Read length
    long  len = static_cast<long>(input.readUint(8)) << 8;   len |= input.readUint(8);
    long nlen = static_cast<long>(input.readUint(8)) << 8;  nlen |= input.readUint(8);
    if ((len ^ 0xFFFF) != nlen)
        throw domain_error("Invalid length in uncompressed block");
    
    // Copy bytes
    for (long i = 0; i < len; i++) {
        int b = input.readUint(8);  // Byte is aligned
        output.put(static_cast<char>(b));
        dictionary.append(b);
    }
}


void Decompressor::decompressHuffmanBlock(
        const CanonicalCode &litLenCode, const std::optional<CanonicalCode> &distCode) {
    
    while (true) {
        int sym = litLenCode.decodeNextSymbol(input);
        if (sym == 256)  // End of block
            break;
        
        if (sym < 256) {  // Literal byte
            uint8_t b = static_cast<uint8_t>(sym);
            output.put(static_cast<char>(b));
            dictionary.append(b);
        } else {  // Length and distance for copying
            int run = decodeRunLength(sym);
            if (run < 3 || run > 258)
                throw logic_error("Invalid run length");
            if (!distCode.has_value())
                throw domain_error("Length symbol encountered with empty distance code");
            int distSym = distCode->decodeNextSymbol(input);
            long dist = decodeDistance(distSym);
            if (dist < 1 || dist > 32768)
                throw logic_error("Invalid distance");
            dictionary.copy(dist, run, output);
        }
    }
}


int Decompressor::decodeRunLength(int sym) {
    // Symbols outside the range cannot occur in the bit stream;
    // they would indicate that the decompressor is buggy
    assert(257 <= sym && sym <= 287);
    
    if (sym <= 264)
        return sym - 254;
    else if (sym <= 284) {
        int numExtraBits = (sym - 261) / 4;
        return (((sym - 265) % 4 + 4) << numExtraBits) + 3 + input.readUint(numExtraBits);
    } else if (sym == 285)
        return 258;
    else  // sym is 286 or 287
        throw domain_error("Reserved length symbol");
}


long Decompressor::decodeDistance(int sym) {
    // Symbols outside the range cannot occur in the bit stream;
    // they would indicate that the decompressor is buggy
    assert(0 <= sym && sym <= 31);
    
    if (sym <= 3)
        return sym + 1;
    else if (sym <= 29) {
        int numExtraBits = sym / 2 - 1;
        return ((sym % 2 + 2L) << numExtraBits) + 1 + input.readUint(numExtraBits);
    } else  // sym is 30 or 31
        throw domain_error("Reserved distance symbol");
}


class DataInput final {
    
    private: std::istream &input;
    
    
    public: DataInput(std::istream &in) :
        input(in) {}
    
    
    public: uint8_t readUint8() {
        int b = input.get();
        if (b == std::char_traits<char>::eof())
            throw std::runtime_error("Unexpected end of stream");
        return static_cast<uint8_t>(b);
    }
    
    
    public: uint16_t readLittleEndianUint16() {
        uint16_t result = 0;
        for (int i = 0; i < 2; i++)
            result |= static_cast<uint16_t>(readUint8()) << (i * 8);
        return result;
    }
    
    
    public: uint32_t readLittleEndianUint32() {
        uint32_t result = 0;
        for (int i = 0; i < 4; i++)
            result |= static_cast<uint32_t>(readUint8()) << (i * 8);
        return result;
    }
    
    
    public: string readNullTerminatedString() {
        string result;
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


static string toHex(uint32_t val, int digits) {
    std::ostringstream s;
    s << std::hex << std::setw(digits) << std::setfill('0') << val;
    return s.str();
}


static string submain(int argc, char *argv[]) {
    // Handle command line arguments
    if (argc != 3)
        return string("Usage: ") + argv[0] + " GzipDecompress InputFile.gz OutputFile";
    const char *inFile = argv[1];
    if (!std::filesystem::exists(inFile))
        return string("Input file does not exist: ") + inFile;
    if (std::filesystem::is_directory(inFile))
        return string("Input file is a directory: ") + inFile;
    const char *outFile = argv[2];
    
    try {
        std::vector<uint8_t> decomp;
        uint32_t crc, size;
        
        // Start reading
        {
            std::ifstream in0(inFile);
            DataInput in1(in0);
            
            // Header
            std::bitset<8> flags;
            {
                if (in1.readLittleEndianUint16() != 0x8B1F)
                    return "Invalid GZIP magic number";
                int compMeth = in1.readUint8();
                if (compMeth != 8)
                    return string("Unsupported compression method: ") + std::to_string(compMeth);
                flags = in1.readUint8();
                
                // Reserved flags
                if (flags[5] || flags[6] || flags[7])
                    return "Reserved flags are set";
                
                // Modification time
                uint32_t mtime = in1.readLittleEndianUint32();
                if (mtime != 0)
                    std::cout << "Last modified: " << mtime << " (Unix time)" << std::endl;
                else
                    std::cout << "Last modified: N/A";
                
                // Extra flags
                std::cout << "Extra flags: ";
                int extraFlags = in1.readUint8();
                switch (extraFlags) {
                    case 2:   std::cout << "Maximum compression";  break;
                    case 4:   std::cout << "Fastest compression";  break;
                    default:  std::cout << "Unknown (" << extraFlags << ")";  break;
                }
                std::cout << std::endl;
                
                // Operating system
                int operatingSystem = in1.readUint8();
                string os;
                switch (operatingSystem) {
                    case   0:  os = "FAT";           break;
                    case   1:  os = "Amiga";         break;
                    case   2:  os = "VMS";           break;
                    case   3:  os = "Unix";          break;
                    case   4:  os = "VM/CMS";        break;
                    case   5:  os = "Atari TOS";     break;
                    case   6:  os = "HPFS";          break;
                    case   7:  os = "Macintosh";     break;
                    case   8:  os = "Z-System";      break;
                    case   9:  os = "CP/M";          break;
                    case  10:  os = "TOPS-20";       break;
                    case  11:  os = "NTFS";          break;
                    case  12:  os = "QDOS";          break;
                    case  13:  os = "Acorn RISCOS";  break;
                    case 255:  os = "Unknown";       break;
                    default :  os = string("Really unknown (") + std::to_string(operatingSystem) + ")";  break;
                }
                std::cout << "Operating system: " << os << std::endl;
            }
            
            // Handle assorted flags
            if (flags[0])
                std::cout << "Flag: Text" << std::endl;
            if (flags[2]) {
                std::cout << "Flag: Extra" << std::endl;
                long len = in1.readLittleEndianUint16();
                for (long i = 0; i < len; i++)  // Skip extra data
                    in1.readUint8();
            }
            if (flags[3])
                std::cout << "File name: " + in1.readNullTerminatedString() << std::endl;
            if (flags[4])
                std::cout << "Comment: " + in1.readNullTerminatedString() << std::endl;
            if (flags[1])
                std::cout << "Header CRC-16: " << toHex(in1.readLittleEndianUint16(), 4) << std::endl;
            
            // Decompress
            try {
                BitInputStream in2(in0);
                decomp = Decompressor::decompress(in2);
            } catch (std::exception &e) {
                return string("Invalid or corrupt compressed data: ") + e.what();
            }
            
            // Footer
            crc  = in1.readLittleEndianUint32();
            size = in1.readLittleEndianUint32();
        }
        
        // Check decompressed data's length and CRC
        if (size != static_cast<uint32_t>(decomp.size()))
            return string("Size mismatch: expected=") + std::to_string(size) + ", actual=" + std::to_string(decomp.size());
        if (crc != getCrc32(decomp))
            return string("CRC-32 mismatch: expected=") + toHex(crc, 8) + ", actual=" + toHex(getCrc32(decomp), 8);
        
        // Write decompressed data to output file
        std::ofstream out(outFile);
        for (uint8_t b : decomp)
            out.put(b);
        
        // Success, no error message
        return "";
        
    } catch (std::exception &e) {
        return string("I/O exception: ") + e.what();
    }
}


/* 
 * Decompression application for the gzip file format.
 * Usage: GzipDecompress InputFile.gz OutputFile
 * This decompresses a single gzip input file into a single output file. The program also prints
 * some information to standard output, and error messages if the file is invalid/corrupt.
 */
int main(int argc, char *argv[]) {
    string msg = submain(argc, argv);
    if (msg.length() == 0)
        return EXIT_SUCCESS;
    else {
        std::cerr << msg << std::endl;
        return EXIT_FAILURE;
    }
}
