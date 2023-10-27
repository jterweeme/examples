/* 
 * Simple DEFLATE decompressor (Java)
 * 
 * Copyright (c) Project Nayuki
 * MIT License. See readme file.
 * https://www.nayuki.io/page/simple-deflate-decompressor
 */

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
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

class GzipDecompress
{
    static class ByteBitInputStream implements java.io.Closeable
    {
        InputStream input;
        int currentByte;
        int numBitsRemaining;

        public int readUint(int numBits) throws IOException
        {
            if (numBits < 0 || numBits > 31)
                throw new IllegalArgumentException("Number of bits out of range");
            int result = 0;
            for (int i = 0; i < numBits; i++)
            {
                int bit = readBitMaybe();
                if (bit == -1)
                    throw new EOFException("Unexpected end of stream");
                result |= bit << i;
            }
            return result;
        }
   
        public ByteBitInputStream(InputStream in)
        {
            input = Objects.requireNonNull(in);
            currentByte = 0;
            numBitsRemaining = 0;
        }
    
        public int getBitPosition()
        {
            assert 0 <= numBitsRemaining && numBitsRemaining < 8 : "Unreachable state";
            return (8 - numBitsRemaining) % 8;
        }
    
        public int readBitMaybe() throws IOException
        {
            if (currentByte == -1)
                return -1;
            if (numBitsRemaining == 0)
            {
                currentByte = input.read();
                if (currentByte == -1)
                    return -1;
                numBitsRemaining = 8;
            }
            assert numBitsRemaining > 0 : "Unreachable state";
            numBitsRemaining--;
            return (currentByte >>> (7 - numBitsRemaining)) & 1;
        }
    
        public void close() throws IOException
        {
            input.close();
            currentByte = -1;
            numBitsRemaining = 0;
        }
    }

    static class CRCOutputStream extends OutputStream
    {
        OutputStream _os;
        int _cnt = 0;
        CRC32 _crc = new CRC32();

        public CRCOutputStream(OutputStream os) { _os = os; }
        public int crc() { return (int)_crc.getValue(); }
        public int cnt() { return _cnt; }

        public void write(int b) throws IOException
        {
            _cnt++;
            _crc.update(b);
            _os.write(b);
        }
    }

    static class CanonicalCode
    {
        int[] symbolCodeBits, symbolValues;
    
        public CanonicalCode(int[] codeLengths)
        {
            Objects.requireNonNull(codeLengths);
            for (int x : codeLengths)
            {
                if (x < 0)
                    throw new IllegalArgumentException("Negative code length");
                if (x > MAX_CODE_LENGTH)
                    throw new IllegalArgumentException("Maximum code length exceeded");
            }
        
            symbolCodeBits = new int[codeLengths.length];
            symbolValues   = new int[codeLengths.length];
            int numSymbolsAllocated = 0;
            int nextCode = 0;
            for (int codeLength = 1; codeLength <= MAX_CODE_LENGTH; codeLength++)
            {
                nextCode <<= 1;
                int startBit = 1 << codeLength;
                for (int symbol = 0; symbol < codeLengths.length; symbol++)
                {
                    if (codeLengths[symbol] != codeLength)
                        continue;
                    if (nextCode >= startBit)
                        throw new IllegalArgumentException(
                            "This canonical code produces an over-full Huffman code tree");
                
                    symbolCodeBits[numSymbolsAllocated] = startBit | nextCode;
                    symbolValues  [numSymbolsAllocated] = symbol;
                    numSymbolsAllocated++;
                    nextCode++;
                }
            }
            if (nextCode != 1 << MAX_CODE_LENGTH)
            {
                throw new IllegalArgumentException(
                    "This canonical code produces an under-full Huffman code tree");
            }

            symbolCodeBits = Arrays.copyOf(symbolCodeBits, numSymbolsAllocated);
            symbolValues   = Arrays.copyOf(symbolValues  , numSymbolsAllocated);
        }

        public int decodeNextSymbol(ByteBitInputStream in) throws IOException
        {
            Objects.requireNonNull(in);
            int codeBits = 1;
            while (true)
            {
                codeBits = codeBits << 1 | in.readUint(1);
                int index = Arrays.binarySearch(symbolCodeBits, codeBits);
                if (index >= 0)
                    return symbolValues[index];
            }
        }
    
        @Override public String toString()
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < symbolCodeBits.length; i++)
            {
                sb.append(String.format("Code %s: Symbol %d%n",
                    Integer.toBinaryString(symbolCodeBits[i]).substring(1),
                    symbolValues[i]));
            }
            return sb.toString();
        }
    
        private static final int MAX_CODE_LENGTH = 15;
    }

    static final class ByteHistory
    {
        byte[] data;
        int index;
        int length;
    
        public ByteHistory(int size)
        {
            if (size < 1)
                throw new IllegalArgumentException("Size must be positive");
            data = new byte[size];
            index = 0;
            length = 0;
        }
    
        public void append(int b)
        {
            assert 0 <= index && index < data.length : "Unreachable state";
            data[index] = (byte)b;
            index = (index + 1) % data.length;
            if (length < data.length)
                length++;
        }
    
        public void copy(int dist, int len, OutputStream out) throws IOException
        {
            Objects.requireNonNull(out);
            if (len < 0 || dist < 1 || dist > length)
                throw new IllegalArgumentException("Invalid length or distance");
        
            int readIndex = (index - dist + data.length) % data.length;
            assert 0 <= readIndex && readIndex < data.length : "Unreachable state";
        
            for (int i = 0; i < len; i++)
            {
                byte b = data[readIndex];
                readIndex = (readIndex + 1) % data.length;
                out.write(b);
                append(b);
            }
        }
    }
    
    static public final class Decompressor
    {
        public static void
        decompress(ByteBitInputStream in, OutputStream out)
            throws IOException, DataFormatException
        {
            new Decompressor(in, out);
        }
    
        ByteBitInputStream input;
        OutputStream output;
        ByteHistory dictionary;
    
        private
        Decompressor(ByteBitInputStream in, OutputStream out)
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
            }
            while (!isFinal);
        }
    
    
        /*-- Constants: The code trees for static Huffman codes (btype = 1) --*/
        private static final CanonicalCode FIXED_LITERAL_LENGTH_CODE;
        private static final CanonicalCode FIXED_DISTANCE_CODE;
    
        static
        {
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
        
            int[] codeLens = new int[numLitLenCodes + numDistCodes];
            for (int codeLensIndex = 0; codeLensIndex < codeLens.length; )
            {
                int sym = codeLenCode.decodeNextSymbol(input);
                if (0 <= sym && sym <= 15)
                {
                    codeLens[codeLensIndex] = sym;
                    codeLensIndex++;
                }
                else
                {
                    int runLen;
                    int runVal = 0;
                    if (sym == 16)
                    {
                        if (codeLensIndex == 0)
                            throw new DataFormatException("No code length value to copy");
                        runLen = input.readUint(2) + 3;
                        runVal = codeLens[codeLensIndex - 1];
                    } else if (sym == 17) {
                        runLen = input.readUint(3) + 3;
                    }
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
                for (int x : distCodeLen)
                {
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
            
                if (sym < 256)
                {  // Literal byte
                    output.write(sym);
                    dictionary.append(sym);
                }
                else
                {   // Length and distance for copying
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
    
        private int decodeRunLength(int sym) throws IOException, DataFormatException
        {
            assert 257 <= sym && sym <= 287 : "Invalid run length symbol: " + sym;
        
            if (sym <= 264)
                return sym - 254;

            if (sym <= 284)
            {
                int numExtraBits = (sym - 261) / 4;
                return (((sym - 265) % 4 + 4) << numExtraBits) + 3 + input.readUint(numExtraBits);
            }

            if (sym == 285)
                return 258;

            throw new DataFormatException("Reserved length symbol: " + sym);
        }
    
        private int decodeDistance(int sym) throws IOException, DataFormatException
        {
            assert 0 <= sym && sym <= 31 : "Invalid distance symbol: " + sym;
        
            if (sym <= 3)
                return sym + 1;

            if (sym <= 29)
            {
                int numExtraBits = sym / 2 - 1;
                return ((sym % 2 + 2) << numExtraBits) + 1 + input.readUint(numExtraBits);
            }

            throw new DataFormatException("Reserved distance symbol: " + sym);
        }
    }
    
    private static String readNullTerminatedString(DataInput in) throws IOException
    {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        while (true)
        {
            byte b = in.readByte();
            if (b == 0)
                break;
            bout.write(b);
        }
        return new String(bout.toByteArray(), StandardCharsets.UTF_8);
    }
    
    
    private static int getCrc32(byte[] data)
    {
        CRC32 crc = new CRC32();
        crc.update(data);
        return (int)crc.getValue();
    }
    
    
    private static int readLittleEndianUint16(DataInput in) throws IOException
    {
        return Integer.reverseBytes(in.readUnsignedShort()) >>> 16;
    }
    
    
    private static int readLittleEndianInt32(DataInput in) throws IOException
    {
        return Integer.reverseBytes(in.readInt());
    }

    static String submain(String[] args)
    {
        // Handle command line arguments
        if (args.length != 2)
            return "Usage: java GzipDecompress InputFile.gz OutputFile";
        File inFile = new File(args[0]);
        if (!inFile.exists())
            return "Input file does not exist: " + inFile;
        if (inFile.isDirectory())
            return "Input file is a directory: " + inFile;
        //Path outFile = Paths.get(args[1]);
        File outFile = new File(args[1]);
        
        try
        {
            DataInputStream in = new DataInputStream(
                    new BufferedInputStream(new FileInputStream(inFile), 16 * 1024));

            if (in.readUnsignedShort() != 0x1F8B)
                return "Invalid GZIP magic number";
            int compMeth = in.readUnsignedByte();
            if (compMeth != 8)
                return "Unsupported compression method: " + compMeth;
            var flagByte = new byte[1];
            in.readFully(flagByte);
            BitSet flags = BitSet.valueOf(flagByte);
            
            // Reserved flags
            if (flags.get(5) || flags.get(6) || flags.get(7))
                return "Reserved flags are set";
            
            // Modification time
            int mtime = readLittleEndianInt32(in);
            if (mtime != 0)
                System.out.println("Last modified: " + Instant.EPOCH.plusSeconds(mtime));
            else
                System.out.println("Last modified: N/A");
            
            in.readUnsignedByte();
            in.readUnsignedByte();
            
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
 
            //ByteArrayOutputStream out = new ByteArrayOutputStream();
            FileOutputStream out = new FileOutputStream(outFile);
            BufferedOutputStream bufOut = new BufferedOutputStream(out);
            CRCOutputStream output = new CRCOutputStream(bufOut);
            Decompressor.decompress(new ByteBitInputStream(in), output);
            //byte[] decomp = out.toByteArray();
            int crc  = readLittleEndianInt32(in);
            int size = readLittleEndianInt32(in);
            System.err.println("size: " + size + " " + output.cnt());
            System.err.println("crc: " + crc + " " + output.crc());
            
            // Write decompressed data to output file
            //Files.write(outFile, decomp);
            
        } catch (IOException e) {
            return "I/O exception: " + e.getMessage();
        } catch (DataFormatException e)
        {
            return "Invalid or corrupt compressed data: " + e.getMessage();
        }
        return null;  // Success, no error message
    }

    public static void main(String[] args)
    {
        String msg = submain(args);

        if (msg != null)
        {
            System.err.println(msg);
            System.exit(1);
        }
    }
}
