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
import java.util.Arrays;
import java.util.BitSet;
import java.util.Objects;
import java.util.zip.CRC32;
import java.util.zip.DataFormatException;

class GzipDecompress
{
    class ByteBitInputStream implements java.io.Closeable
    {
        InputStream _input;
        int _currentByte = 0;
        int _numBitsRemaining = 0;
        ByteBitInputStream(InputStream in) { _input = in; }
        int getBitPosition() { return (8 - _numBitsRemaining) % 8; }

        int _readBitMaybe() throws IOException
        {
            if (_currentByte == -1)
                return -1;
            if (_numBitsRemaining == 0)
            {
                _currentByte = _input.read();
                if (_currentByte == -1)
                    return -1;
                _numBitsRemaining = 8;
            }
            _numBitsRemaining--;
            return (_currentByte >>> (7 - _numBitsRemaining)) & 1;
        }

        int readUint(int numBits) throws IOException
        {
            int result = 0;
            for (int i = 0; i < numBits; i++)
                result |= _readBitMaybe() << i;
            return result;
        }
    
        public void close() throws IOException
        {
            _input.close();
            _currentByte = -1;
            _numBitsRemaining = 0;
        }
    }

    class CRCOutputStream extends OutputStream
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
        int _numSymbolsAllocated = 0;
        static final int MAX_CODE_LENGTH = 15;
    
        CanonicalCode(int[] codeLengths)
        {
            symbolCodeBits = new int[codeLengths.length];
            symbolValues = new int[codeLengths.length];
            for (int codeLength = 1, nextCode = 0; codeLength <= MAX_CODE_LENGTH; codeLength++)
            {
                nextCode <<= 1;
                int startBit = 1 << codeLength;
                for (int symbol = 0; symbol < codeLengths.length; symbol++)
                {
                    if (codeLengths[symbol] != codeLength)
                        continue;
                    symbolCodeBits[_numSymbolsAllocated] = startBit | nextCode;
                    symbolValues  [_numSymbolsAllocated] = symbol;
                    _numSymbolsAllocated++;
                    nextCode++;
                }
            }
        }

        int decodeNextSymbol(ByteBitInputStream in) throws IOException
        {
            for (int codeBits = 1; true;)
            {
                codeBits = codeBits << 1 | in.readUint(1);
                int index = Arrays.binarySearch(symbolCodeBits, 0, _numSymbolsAllocated, codeBits);
                if (index >= 0)
                    return symbolValues[index];
            }
        }
    }

    class ByteHistory
    {
        byte[] data;
        int index = 0, length = 0;
        ByteHistory(int size) { data = new byte[size]; }
    
        void append(int b)
        {
            data[index] = (byte)b;
            index = (index + 1) % data.length;
            if (length < data.length) length++;
        }
    
        void copy(int dist, int len, OutputStream out) throws IOException
        {
            int readIndex = (index - dist + data.length) % data.length;
            for (int i = 0; i < len; i++)
            {
                byte b = data[readIndex];
                readIndex = (readIndex + 1) % data.length;
                out.write(b);
                append(b);
            }
        }
    }
    
    class Decompressor
    {
        final ByteBitInputStream input;
        final OutputStream output;
        final ByteHistory dictionary;
        CanonicalCode FIXED_LITERAL_LENGTH_CODE;
        CanonicalCode FIXED_DISTANCE_CODE;

        Decompressor(ByteBitInputStream in, OutputStream out)
            throws IOException, DataFormatException
        {
            input = Objects.requireNonNull(in);
            output = Objects.requireNonNull(out);
            dictionary = new ByteHistory(32 * 1024);
            int[] llcodelens = new int[288];
            int i = 0;
            for (; i < 144; i++) llcodelens[i] = 8;
            for (; i < 256; i++) llcodelens[i] = 9;
            for (; i < 280; i++) llcodelens[i] = 7;
            for (; i < 288; i++) llcodelens[i] = 8;
            FIXED_LITERAL_LENGTH_CODE = new CanonicalCode(llcodelens);
            int[] distcodelens = new int[32];
            Arrays.fill(distcodelens, 5);
            FIXED_DISTANCE_CODE = new CanonicalCode(distcodelens);
        }

        void decompress() throws IOException, DataFormatException
        {
            for (boolean isFinal = false; !isFinal;)
            {
                isFinal = input.readUint(1) != 0;  // bfinal
                int type = input.readUint(2);  // btype
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
        }

        private CanonicalCode[] decodeHuffmanCodes() throws IOException, DataFormatException
        {
            int numLitLenCodes = input.readUint(5) + 257;  // hlit + 257
            int numDistCodes = input.readUint(5) + 1;      // hdist + 1
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
        
            CanonicalCode codeLenCode = new CanonicalCode(codeLenCodeLen);
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
                    int runLen, runVal = 0;
                    if (sym == 16)
                    {
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
                    Arrays.fill(codeLens, codeLensIndex, end, runVal);
                    codeLensIndex = end;
                }
            }
        
            int[] litLenCodeLen = Arrays.copyOf(codeLens, numLitLenCodes);
            CanonicalCode litLenCode = new CanonicalCode(litLenCodeLen);
            int[] distCodeLen = Arrays.copyOfRange(codeLens, numLitLenCodes, codeLens.length);
            CanonicalCode distCode;
            if (distCodeLen.length == 1 && distCodeLen[0] == 0)
            {
                //distCode = null;
                distCode = new CanonicalCode(distCodeLen);  //bogus CanonicalCode
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
                    distCodeLen = Arrays.copyOf(distCodeLen, 32);
                    distCodeLen[31] = 1;
                }
                distCode = new CanonicalCode(distCodeLen);
            }
        
            return new CanonicalCode[]{litLenCode, distCode};
        }
    
        void decompressUncompressedBlock() throws IOException, DataFormatException
        {
            while (input.getBitPosition() != 0)
                input.readUint(1);
            int  len = input.readUint(16);
            int nlen = input.readUint(16);
            if ((len ^ 0xFFFF) != nlen)
                throw new DataFormatException("Invalid length in uncompressed block");
            for (int i = 0; i < len; i++)
            {
                int b = input.readUint(8);  // Byte is aligned
                output.write(b);
                dictionary.append(b);
            }
        }
    
        void decompressHuffmanBlock(CanonicalCode litLenCode, CanonicalCode distCode)
            throws IOException, DataFormatException
        {
            for (int sym; (sym = litLenCode.decodeNextSymbol(input)) != 256;)
            {
                if (sym < 256)
                {
                    output.write(sym);
                    dictionary.append(sym);
                }
                else
                {
                    int run = decodeRunLength(sym);
                    int distSym = distCode.decodeNextSymbol(input);
                    int dist = decodeDistance(distSym);
                    dictionary.copy(dist, run, output);
                }
            }
        }
    
        int decodeRunLength(int sym) throws IOException, DataFormatException
        {
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
    
        int decodeDistance(int sym) throws IOException, DataFormatException
        {
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
    
    String readNullTerminatedString(DataInput in) throws IOException
    {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        for (byte b; (b = in.readByte()) != 0;)
            bout.write(b);

        return new String(bout.toByteArray(), StandardCharsets.UTF_8);
    }
    
    int readLittleEndianUint16(DataInput in) throws IOException
    {   return Integer.reverseBytes(in.readUnsignedShort()) >>> 16;
    }
    
    int readLittleEndianInt32(DataInput in) throws IOException
    {   return Integer.reverseBytes(in.readInt());
    }

    String submain(File inFile, OutputStream os, java.io.PrintStream msgOut)
    {
        try
        {
            final DataInputStream in = new DataInputStream(
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
                msgOut.println("Last modified: " + java.time.Instant.EPOCH.plusSeconds(mtime));
            else
                msgOut.println("Last modified: N/A");
            
            in.readUnsignedByte();
            in.readUnsignedByte();
            
            if (flags.get(0))
                msgOut.println("Flag: Text");
            if (flags.get(2)) {
                msgOut.println("Flag: Extra");
                in.skipNBytes(readLittleEndianUint16(in));
            }
            if (flags.get(3))
                msgOut.println("File name: " + readNullTerminatedString(in));
            if (flags.get(4))
                msgOut.println("Comment: " + readNullTerminatedString(in));
            if (flags.get(1))
                msgOut.printf("Header CRC-16: %04X%n", readLittleEndianUint16(in));
 
            CRCOutputStream output = new CRCOutputStream(os);
            Decompressor dec = new Decompressor(new ByteBitInputStream(in), output);
            dec.decompress();
            os.flush();
            int crc  = readLittleEndianInt32(in);
            int size = readLittleEndianInt32(in);
            msgOut.println("size: " + size + " " + output.cnt());
            msgOut.println("crc: " + crc + " " + output.crc());
        } catch (IOException e) {
            return "I/O exception: " + e.getMessage();
        } catch (DataFormatException e)
        {
            return "Invalid or corrupt compressed data: " + e.getMessage();
        }
        return null;
    }

    public static void main(String[] args)
    {
        if (args.length != 1)
        {
            System.err.println("Usage: java GzipDecompress InputFile.gz OutputFile");
            System.exit(1);
        }
        String msg = new GzipDecompress().submain(new File(args[0]), System.out, System.err);

        if (msg != null)
        {
            System.err.println(msg);
            System.exit(1);
        }
    }
}
