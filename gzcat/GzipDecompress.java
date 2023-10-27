/* 
 * Simple DEFLATE decompressor (Java)
 * 
 * Copyright (c) Project Nayuki
 * MIT License. See readme file.
 * https://www.nayuki.io/page/simple-deflate-decompressor
 */

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Instant;
import java.util.Arrays;
import java.util.BitSet;
import java.util.Objects;
import java.util.zip.CRC32;
import java.util.zip.DataFormatException;


/**
 * Decompression application for the gzip file format.
 * <p>Usage: java GzipDecompress InputFile.gz OutputFile</p>
 * <p>This decompresses a single gzip input file into
 * a single output file. The program also prints
 * some information to standard output, and error messages if the file is invalid/corrupt.</p>
 */
public final class GzipDecompress
{
    /**
 * A stream of bits that can be read. Bits are packed in little endian within a byte.
 * For example, the byte 0x87 reads as the sequence of bits [1,1,1,0,0,0,0,1].
 */
public interface BitInputStream extends Closeable {
    
    /**
     * Returns the current bit position, which ascends from 0 to 7 as bits are read.
     * @return the current bit position, which is between 0 and 7
     */
    public int getBitPosition();
    
    
    /**
     * Reads a bit from this stream. Returns 0 or 1 if a bit is available, or -1 if
     * the end of stream is reached. The end of stream always occurs on a byte boundary.
     * @return the next bit of 0 or 1, or -1 for the end of stream
     * @throws IOException if an I/O exception occurs
     */
    public int readBitMaybe() throws IOException;
    
    
    /**
     * Reads the specified number of bits from this stream,
     * packing them in little endian as an unsigned integer.
     * @param numBits the number of bits to read, in the range [0, 31]
     * @return a number in the range [0, 2<sup>numBits</sup>)
     * @throws IllegalArgumentException if the number of bits is out of range
     * @throws IOException if an I/O exception occurs
     * @throws EOFException if the end of stream is reached
     */
    public default int readUint(int numBits) throws IOException {
        if (numBits < 0 || numBits > 31)
            throw new IllegalArgumentException("Number of bits out of range");
        int result = 0;
        for (int i = 0; i < numBits; i++) {
            int bit = readBitMaybe();
            if (bit == -1)
                throw new EOFException("Unexpected end of stream");
            result |= bit << i;
        }
        return result;
    }
    
    
    /**
     * Closes this stream and the underlying input stream.
     * @throws IOException if an I/O exception occurs
     */
    @Override public void close() throws IOException;
    
}
    
    /**
     * A canonical Huffman code, where the code values for each symbol is
     * derived from a given sequence of code lengths. This data structure is
     * immutable. This could be transformed into an explicit Huffman code tree.
     * <p>Example:</p>
     * <pre>  Code lengths (canonical code):
     *    Symbol A: 1
     *    Symbol B: 0 (no code)
     *    Symbol C: 3
     *    Symbol D: 2
     *    Symbol E: 3
     *  
     *  Generated Huffman codes:
     *    Symbol A: 0
     *    Symbol B: (Absent)
     *    Symbol C: 110
     *    Symbol D: 10
     *    Symbol E: 111
     *  
     *  Huffman code tree:
     *      .
     *     / \
     *    A   .
     *       / \
     *      D   .
     *         / \
     *        C   E</pre>
     */
    static final class CanonicalCode
    {
    
    /* 
     * These arrays store the Huffman codes and values necessary for decoding.
     * symbolCodeBits contains Huffman codes, each padded with a 1 bit at the
     * beginning to disambiguate codes of different lengths (e.g. otherwise we
     * can't distinguish 0b01 from 0b0001). Each symbolCodeBits[i] decodes to its
     * corresponding symbolValues[i]. Values in symbolCodeBits are strictly increasing.
     * 
     * For the example of codeLengths=[1,0,3,2,3], we would have:
     *   i | symbolCodeBits[i] | symbolValues[i]
     *   --+-------------------+----------------
     *   0 |             0b1_0 |               0
     *   1 |            0b1_10 |               3
     *   2 |           0b1_110 |               2
     *   3 |           0b1_111 |               4
     */
    private int[] symbolCodeBits;
    private int[] symbolValues;
    
    
    
    /*---- Constructor ----*/
    
    /**
     * Constructs a canonical Huffman code from the specified array of symbol code lengths.
     * Each code length must be non-negative. Code length 0 means no code for the symbol.
     * The collection of code lengths must represent a proper full Huffman code tree.
     * <p>Examples of code lengths that result in correct full Huffman code trees:</p>
     * <ul>
     *   <li>[1, 1] (result: A=0, B=1)</li>
     *   <li>[2, 2, 1, 0, 0, 0] (result: A=10, B=11, C=0)</li>
     *   <li>[3, 3, 3, 3, 3, 3, 3, 3] (result: A=000, B=001, C=010, ..., H=111)</li>
     * </ul>
     * <p>Examples of code lengths that result in under-full Huffman code trees:</p>
     * <ul>
     *   <li>[0, 2, 0] (result: B=00, unused=01, unused=1)</li>
     *   <li>[0, 1, 0, 2] (result: B=0, D=10, unused=11)</li>
     * </ul>
     * <p>Examples of code lengths that result in over-full Huffman code trees:</p>
     * <ul>
     *   <li>[1, 1, 1] (result: A=0, B=1, C=overflow)</li>
     *   <li>[1, 1, 2, 2, 3, 3, 3, 3] (result: A=0, B=1, C=overflow, ...)</li>
     * </ul>
     * @param canonicalCodeLengths array of symbol code lengths (not {@code null})
     * @throws NullPointerException if the array is {@code null}
     * @throws IllegalArgumentException if any element is negative, any value exceeds MAX_CODE_LENGTH,
     * or the collection of code lengths would yield an under-full or over-full Huffman code tree
     */
    public CanonicalCode(int[] codeLengths) {
        // Check argument values
        Objects.requireNonNull(codeLengths);
        for (int x : codeLengths) {
            if (x < 0)
                throw new IllegalArgumentException("Negative code length");
            if (x > MAX_CODE_LENGTH)
                throw new IllegalArgumentException("Maximum code length exceeded");
        }
        
        // Allocate code values to symbols. Symbols are processed in the order
        // of shortest code length first, breaking ties by lowest symbol value.
        symbolCodeBits = new int[codeLengths.length];
        symbolValues   = new int[codeLengths.length];
        int numSymbolsAllocated = 0;
        int nextCode = 0;
        for (int codeLength = 1; codeLength <= MAX_CODE_LENGTH; codeLength++) {
            nextCode <<= 1;
            int startBit = 1 << codeLength;
            for (int symbol = 0; symbol < codeLengths.length; symbol++) {
                if (codeLengths[symbol] != codeLength)
                    continue;
                if (nextCode >= startBit)
                    throw new IllegalArgumentException("This canonical code produces an over-full Huffman code tree");
                
                symbolCodeBits[numSymbolsAllocated] = startBit | nextCode;
                symbolValues  [numSymbolsAllocated] = symbol;
                numSymbolsAllocated++;
                nextCode++;
            }
        }
        if (nextCode != 1 << MAX_CODE_LENGTH)
            throw new IllegalArgumentException("This canonical code produces an under-full Huffman code tree");
        
        // Trim unused trailing elements
        symbolCodeBits = Arrays.copyOf(symbolCodeBits, numSymbolsAllocated);
        symbolValues   = Arrays.copyOf(symbolValues  , numSymbolsAllocated);
    }
    
    
    
    /*---- Methods ----*/
    
    /**
     * Decodes the next symbol from the specified bit input stream based on this
     * canonical code. The returned symbol value is in the range [0, codeLengths.length).
     * @param in the bit input stream to read from
     * @return the next decoded symbol
     * @throws IOException if an I/O exception occurs
     */
    public int decodeNextSymbol(BitInputStream in) throws IOException {
        Objects.requireNonNull(in);
        int codeBits = 1;  // The start bit
        while (true) {
            // Accumulate one bit at a time on the right side until a match is found
            // in the symbolCodeBits array. Because the Huffman code tree is full,
            // this loop must terminate after at most MAX_CODE_LENGTH iterations.
            codeBits = codeBits << 1 | in.readUint(1);
            int index = Arrays.binarySearch(symbolCodeBits, codeBits);
            if (index >= 0)
                return symbolValues[index];
        }
    }
    
    
    /**
     * Returns a string representation of this canonical code,
     * useful for debugging only, and the format is subject to change.
     * @return a string representation of this canonical code
     */
    @Override public String toString() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < symbolCodeBits.length; i++) {
            sb.append(String.format("Code %s: Symbol %d%n",
                Integer.toBinaryString(symbolCodeBits[i]).substring(1),
                symbolValues[i]));
        }
        return sb.toString();
    }
    
    
    
    /*---- Constant ----*/
    
    // The maximum Huffman code length allowed in the DEFLATE standard.
    private static final int MAX_CODE_LENGTH = 15;
    
}

    
    /**
     * Stores a finite recent history of a byte stream. Useful as an implicit
     * dictionary for Lempel-Ziv schemes. Mutable and not thread-safe.
     */
    static final class ByteHistory
    {
    // Circular buffer of byte data.
    private byte[] data;
    
    // Index of next byte to write to, always in the range [0, data.length).
    private int index;
    
    // Number of bytes written, saturating at data.length.
    private int length;
    
    /**
     * Constructs a byte history of the specified size.
     * @param size the size, which must be positive
     * @throws IllegalArgumentException if size is zero or negative
     */
    public ByteHistory(int size)
    {
        if (size < 1)
            throw new IllegalArgumentException("Size must be positive");
        data = new byte[size];
        index = 0;
        length = 0;
    }
    
    /**
     * Appends the specified byte to this history.
     * This overwrites the byte value at {@code size} positions ago.
     * @param b the byte value to append
     */
    public void append(int b)
    {
        assert 0 <= index && index < data.length : "Unreachable state";
        data[index] = (byte)b;
        index = (index + 1) % data.length;
        if (length < data.length)
            length++;
    }
    
    /**
     * Copies {@code len} bytes starting at {@code dist} bytes ago to
     * the specified output stream and also back into this buffer itself.
     * <p>Note that if the length exceeds the distance, then some of the output
     * data will be a copy of data that was copied earlier in the process.</p>
     * @param dist the distance to go back, in the range [1, size]
     * @param len the length to copy, which must be at least 0
     * @param out the output stream to write to (not {@code null})
     * @throws NullPointerException if the output stream is {@code null}
     * @throws IllegalArgumentException if the length is negative,
     * distance is not positive, or distance is greater than the buffer size
     * @throws IOException if an I/O exception occurs
     */
    public void copy(int dist, int len, OutputStream out) throws IOException {
        Objects.requireNonNull(out);
        if (len < 0 || dist < 1 || dist > length)
            throw new IllegalArgumentException("Invalid length or distance");
        
        int readIndex = (index - dist + data.length) % data.length;
        assert 0 <= readIndex && readIndex < data.length : "Unreachable state";
        
        for (int i = 0; i < len; i++) {
            byte b = data[readIndex];
            readIndex = (readIndex + 1) % data.length;
            out.write(b);
            append(b);
        }
    }
    
}
    
    /**
     * A stream of bits that can be read. Because they come from an underlying byte stream,
     * the total number of bits is always a multiple of 8. The bits are read in little endian.
     * Mutable and not thread-safe.
     */
    static public final class ByteBitInputStream implements BitInputStream
    {
        // The underlying byte stream to read from (not null).
        private InputStream input;
    
        // Either in the range [0x00, 0xFF] if bits are
        // available, or -1 if end of stream is reached.
        private int currentByte;
    
        // Number of remaining bits in the current byte, in the range [0, 8).
        private int numBitsRemaining;
    
        /**
         * Constructs a bit input stream based on the specified byte input stream.
         * @param in the byte input stream (not {@code null})
         * @throws NullPointerException if the input stream is {@code null}
         */
        public ByteBitInputStream(InputStream in)
        {
            input = Objects.requireNonNull(in);
            currentByte = 0;
            numBitsRemaining = 0;
        }
    
        @Override public int getBitPosition()
        {
            assert 0 <= numBitsRemaining && numBitsRemaining < 8 : "Unreachable state";
            return (8 - numBitsRemaining) % 8;
        }
    
    
        @Override public int readBitMaybe() throws IOException
        {
        if (currentByte == -1)
            return -1;
        if (numBitsRemaining == 0) {
            currentByte = input.read();
            if (currentByte == -1)
                return -1;
            numBitsRemaining = 8;
        }
        assert numBitsRemaining > 0 : "Unreachable state";
        numBitsRemaining--;
        return (currentByte >>> (7 - numBitsRemaining)) & 1;
        }
    
        @Override public void close() throws IOException {
        input.close();
        currentByte = -1;
        numBitsRemaining = 0;
        }
    
    }
    
    /**
     * Decompresses raw DEFLATE data (without zlib or gzip container) into bytes.
     */
    static public final class Decompressor
    {
        /**
         * Reads from the specified input stream, decompresses
         * the data, and returns a new byte array.
         * @param in the bit input stream to read from (not {@code null})
         * @throws NullPointerException if the input stream is {@code null}
         * @throws DataFormatException if the DEFLATE data is malformed
         */
        public static byte[] decompress(BitInputStream in)
            throws IOException, DataFormatException
        {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            decompress(in, out);
            return out.toByteArray();
        }
    
        /**
         * Reads from the specified input stream, decompresses
         * the data, and writes to the specified output stream.
         * @param in the bit input stream to read from (not {@code null})
         * @param out the byte output stream to write to (not {@code null})
         * @throws NullPointerException if the input or output stream is {@code null}
         * @throws DataFormatException if the DEFLATE data is malformed
         */
        public static void
        decompress(BitInputStream in, OutputStream out)
            throws IOException, DataFormatException
        {
            new Decompressor(in, out);
        }
    
        private BitInputStream input;
        private OutputStream output;
        private ByteHistory dictionary;
    
   
        // Constructor, which immediately performs decompression
        private
        Decompressor(BitInputStream in, OutputStream out)
            throws IOException, DataFormatException
        {
            // Initialize fields
            input = Objects.requireNonNull(in);
            output = Objects.requireNonNull(out);
            dictionary = new ByteHistory(32 * 1024);
        
            // Process the stream of blocks
            boolean isFinal;
            do
            {
            // Read the block header
            isFinal = input.readUint(1) != 0;  // bfinal
            int type = input.readUint(2);  // btype
            
            // Decompress rest of block based on the type
            if (type == 0)
                decompressUncompressedBlock();
            else if (type == 1)
                decompressHuffmanBlock(FIXED_LITERAL_LENGTH_CODE, FIXED_DISTANCE_CODE);
            else if (type == 2) {
                CanonicalCode[] litLenAndDist = decodeHuffmanCodes();
                decompressHuffmanBlock(litLenAndDist[0], litLenAndDist[1]);
            } else if (type == 3)
                throw new DataFormatException("Reserved block type");
            else
                throw new AssertionError("Unreachable value");
            } while (!isFinal);
        }
    
    
        /*-- Constants: The code trees for static Huffman codes (btype = 1) --*/
        private static final CanonicalCode FIXED_LITERAL_LENGTH_CODE;
        private static final CanonicalCode FIXED_DISTANCE_CODE;
    
        static
        {  // Make temporary tables of canonical code lengths
            int[] llcodelens = new int[288];
            Arrays.fill(llcodelens,   0, 144, 8);
            Arrays.fill(llcodelens, 144, 256, 9);
            Arrays.fill(llcodelens, 256, 280, 7);
            Arrays.fill(llcodelens, 280, 288, 8);
            FIXED_LITERAL_LENGTH_CODE = new CanonicalCode(llcodelens);
            
            int[] distcodelens = new int[32];
            Arrays.fill(distcodelens, 5);
            FIXED_DISTANCE_CODE = new CanonicalCode(distcodelens);
        }
    
    
        /*-- Method: Reading and decoding dynamic Huffman codes (btype = 2) --*/
    
        // Reads from the bit input stream, decodes the Huffman code
        // specifications into code trees, and returns the trees.
        private CanonicalCode[] decodeHuffmanCodes() throws IOException, DataFormatException
        {
            int numLitLenCodes = input.readUint(5) + 257;  // hlit + 257
            int numDistCodes = input.readUint(5) + 1;      // hdist + 1
        
            // Read the code length code lengths
            int numCodeLenCodes = input.readUint(4) + 4;   // hclen + 4
            int[] codeLenCodeLen = new int[19];  // This array is filled in a strange order
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
            CanonicalCode codeLenCode;
            try {
                codeLenCode = new CanonicalCode(codeLenCodeLen);
            } catch (IllegalArgumentException e) {
                throw new DataFormatException(e.getMessage());
            }
        
            // Read the main code lengths and handle runs
            int[] codeLens = new int[numLitLenCodes + numDistCodes];
            for (int codeLensIndex = 0; codeLensIndex < codeLens.length; )
            {
                int sym = codeLenCode.decodeNextSymbol(input);
                if (0 <= sym && sym <= 15) {
                    codeLens[codeLensIndex] = sym;
                    codeLensIndex++;
                } else {
                int runLen;
                int runVal = 0;
                if (sym == 16) {
                    if (codeLensIndex == 0)
                        throw new DataFormatException("No code length value to copy");
                    runLen = input.readUint(2) + 3;
                    runVal = codeLens[codeLensIndex - 1];
                } else if (sym == 17)
                    runLen = input.readUint(3) + 3;
                else if (sym == 18)
                    runLen = input.readUint(7) + 11;
                else
                    throw new AssertionError("Symbol out of range");
                int end = codeLensIndex + runLen;
                if (end > codeLens.length)
                    throw new DataFormatException("Run exceeds number of codes");
                Arrays.fill(codeLens, codeLensIndex, end, runVal);
                codeLensIndex = end;
                }
            }
        
            // Create literal-length code tree
            int[] litLenCodeLen = Arrays.copyOf(codeLens, numLitLenCodes);
            if (litLenCodeLen[256] == 0)
                throw new DataFormatException("End-of-block symbol has zero code length");
            CanonicalCode litLenCode;
            try {
            litLenCode = new CanonicalCode(litLenCodeLen);
            } catch (IllegalArgumentException e) {
            throw new DataFormatException(e.getMessage());
            }
        
            // Create distance code tree with some extra processing
            int[] distCodeLen = Arrays.copyOfRange(codeLens, numLitLenCodes, codeLens.length);
            CanonicalCode distCode;
            if (distCodeLen.length == 1 && distCodeLen[0] == 0)
            {
                distCode = null;  // Empty distance code; the block shall be all literal symbols
            }
            else
            {
                // Get statistics for upcoming logic
                int oneCount = 0;
                int otherPositiveCount = 0;
                for (int x : distCodeLen) {
                if (x == 1)
                    oneCount++;
                else if (x > 1)
                    otherPositiveCount++;
                }
            
                // Handle the case where only one distance code is defined
                if (oneCount == 1 && otherPositiveCount == 0) {
                // Add a dummy invalid code to make the Huffman tree complete
                distCodeLen = Arrays.copyOf(distCodeLen, 32);
                distCodeLen[31] = 1;
                }
                try {
                distCode = new CanonicalCode(distCodeLen);
                } catch (IllegalArgumentException e) {
                throw new DataFormatException(e.getMessage());
            }
            }
        
            return new CanonicalCode[]{litLenCode, distCode};
        }
    
    
        /*-- Methods: Block decompression --*/
    
        // Handles and copies an uncompressed block from the bit input stream.
        private void decompressUncompressedBlock() throws IOException, DataFormatException
        {
            // Discard bits to align to byte boundary
            while (input.getBitPosition() != 0)
                input.readUint(1);
        
            // Read length
            int  len = input.readUint(16);
            int nlen = input.readUint(16);
            if ((len ^ 0xFFFF) != nlen)
                throw new DataFormatException("Invalid length in uncompressed block");
        
            // Copy bytes
            for (int i = 0; i < len; i++)
            {
                int b = input.readUint(8);  // Byte is aligned
                output.write(b);
                dictionary.append(b);
            }
        }
    
    
        // Decompresses a Huffman-coded block from the bit
        // input stream based on the given Huffman codes.
        private void decompressHuffmanBlock(CanonicalCode litLenCode, CanonicalCode distCode)
            throws IOException, DataFormatException
        {
            Objects.requireNonNull(litLenCode);
            // distCode is allowed to be null
        
            while (true)
            {
                int sym = litLenCode.decodeNextSymbol(input);
                if (sym == 256)  // End of block
                    break;
            
                if (sym < 256) {  // Literal byte
                output.write(sym);
                dictionary.append(sym);
                } else {  // Length and distance for copying
                int run = decodeRunLength(sym);
                assert 3 <= run && run <= 258 : "Invalid run length";
                if (distCode == null)
                {
                    throw new DataFormatException(
                        "Length symbol encountered with empty distance code");
                }
                int distSym = distCode.decodeNextSymbol(input);
                int dist = decodeDistance(distSym);
                assert 1 <= dist && dist <= 32768 : "Invalid distance";
                dictionary.copy(dist, run, output);
                }
            }
        }
    
        /*-- Methods: Symbol decoding --*/
    
        // Returns the run length based on the given symbol and possibly reading more bits.
        private int decodeRunLength(int sym) throws IOException, DataFormatException {
        // Symbols outside the range cannot occur in the bit stream;
        // they would indicate that the decompressor is buggy
        assert 257 <= sym && sym <= 287 : "Invalid run length symbol: " + sym;
        
        if (sym <= 264)
            return sym - 254;
        else if (sym <= 284) {
            int numExtraBits = (sym - 261) / 4;
            return (((sym - 265) % 4 + 4) << numExtraBits) + 3 + input.readUint(numExtraBits);
        } else if (sym == 285)
            return 258;
        else  // sym is 286 or 287
            throw new DataFormatException("Reserved length symbol: " + sym);
        }
    
        // Returns the distance based on the given symbol and possibly reading more bits.
        private int decodeDistance(int sym) throws IOException, DataFormatException {
        // Symbols outside the range cannot occur in the bit stream;
        // they would indicate that the decompressor is buggy
        assert 0 <= sym && sym <= 31 : "Invalid distance symbol: " + sym;
        
        if (sym <= 3)
            return sym + 1;
        else if (sym <= 29) {
            int numExtraBits = sym / 2 - 1;
            return ((sym % 2 + 2) << numExtraBits) + 1 + input.readUint(numExtraBits);
        } else  // sym is 30 or 31
            throw new DataFormatException("Reserved distance symbol: " + sym);
        }
    }
    
    public static void main(String[] args)
    {
        String msg = submain(args);
        if (msg != null) {
            System.err.println(msg);
            System.exit(1);
        }
    }
    
    
    // Returns null if successful, otherwise returns an error message string.
    private static String submain(String[] args) {
        // Handle command line arguments
        if (args.length != 2)
            return "Usage: java GzipDecompress InputFile.gz OutputFile";
        File inFile = new File(args[0]);
        if (!inFile.exists())
            return "Input file does not exist: " + inFile;
        if (inFile.isDirectory())
            return "Input file is a directory: " + inFile;
        Path outFile = Paths.get(args[1]);
        
        try
        {
            byte[] decomp;
            int crc, size;
            // Start reading
            try (DataInputStream in = new DataInputStream(
                    new BufferedInputStream(new FileInputStream(inFile), 16 * 1024)))
            {
                // Header
                BitSet flags;
                {
                    if (in.readUnsignedShort() != 0x1F8B)
                        return "Invalid GZIP magic number";
                    int compMeth = in.readUnsignedByte();
                    if (compMeth != 8)
                        return "Unsupported compression method: " + compMeth;
                    var flagByte = new byte[1];
                    in.readFully(flagByte);
                    flags = BitSet.valueOf(flagByte);
                    
                    // Reserved flags
                    if (flags.get(5) || flags.get(6) || flags.get(7))
                        return "Reserved flags are set";
                    
                    // Modification time
                    int mtime = readLittleEndianInt32(in);
                    if (mtime != 0)
                        System.out.println("Last modified: " + Instant.EPOCH.plusSeconds(mtime));
                    else
                        System.out.println("Last modified: N/A");
                    
                    // Extra flags
                    int extraFlags = in.readUnsignedByte();
                    System.out.println("Extra flags: " + switch (extraFlags) {
                        case 2  -> "Maximum compression";
                        case 4  -> "Fastest compression";
                        default -> "Unknown (" + extraFlags + ")";
                    });
                    
                    // Operating system
                    int operatingSystem = in.readUnsignedByte();
                    String os = switch (operatingSystem) {
                        case   0 -> "FAT";
                        case   1 -> "Amiga";
                        case   2 -> "VMS";
                        case   3 -> "Unix";
                        case   4 -> "VM/CMS";
                        case   5 -> "Atari TOS";
                        case   6 -> "HPFS";
                        case   7 -> "Macintosh";
                        case   8 -> "Z-System";
                        case   9 -> "CP/M";
                        case  10 -> "TOPS-20";
                        case  11 -> "NTFS";
                        case  12 -> "QDOS";
                        case  13 -> "Acorn RISCOS";
                        case 255 -> "Unknown";
                        default  -> "Really unknown (" + operatingSystem + ")";
                    };
                    System.out.println("Operating system: " + os);
                }
                
                // Handle assorted flags
                if (flags.get(0))
                    System.out.println("Flag: Text");
                if (flags.get(2)) {
                    System.out.println("Flag: Extra");
                    in.skipNBytes(readLittleEndianUint16(in));
                }
                if (flags.get(3))
                    System.out.println("File name: " + readNullTerminatedString(in));
                if (flags.get(4))
                    System.out.println("Comment: " + readNullTerminatedString(in));
                if (flags.get(1))
                    System.out.printf("Header CRC-16: %04X%n", readLittleEndianUint16(in));
                
                // Decompress
                try {
                    decomp = Decompressor.decompress(new ByteBitInputStream(in));
                } catch (DataFormatException e) {
                    return "Invalid or corrupt compressed data: " + e.getMessage();
                }
                
                // Footer
                crc  = readLittleEndianInt32(in);
                size = readLittleEndianInt32(in);
            }
            
            // Check decompressed data's length and CRC
            if (size != decomp.length)
            {
                return String.format("Size mismatch: expected=%d, actual=%d",
                                     size, decomp.length);
            }

            if (crc != getCrc32(decomp))
            {
                return String.format("CRC-32 mismatch: expected=%08X, actual=%08X",
                            crc, getCrc32(decomp));
            }
            
            // Write decompressed data to output file
            Files.write(outFile, decomp);
            
        } catch (IOException e) {
            return "I/O exception: " + e.getMessage();
        }
        return null;  // Success, no error message
    }
    
    
    /*---- Helper methods ----*/
    
    private static String readNullTerminatedString(DataInput in) throws IOException {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();
        while (true) {
            byte b = in.readByte();
            if (b == 0)
                break;
            bout.write(b);
        }
        return new String(bout.toByteArray(), StandardCharsets.UTF_8);
    }
    
    
    private static int getCrc32(byte[] data) {
        CRC32 crc = new CRC32();
        crc.update(data);
        return (int)crc.getValue();
    }
    
    
    private static int readLittleEndianUint16(DataInput in) throws IOException {
        return Integer.reverseBytes(in.readUnsignedShort()) >>> 16;
    }
    
    
    private static int readLittleEndianInt32(DataInput in) throws IOException {
        return Integer.reverseBytes(in.readInt());
    }
    
}
