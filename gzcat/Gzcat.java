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
import java.util.zip.DataFormatException;

class Gzcat
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

    class CRC32
    {
        int _table[] = new int[256];
        int _crc = 0xffffffff;

        void _makeTable()
        {
            for (int n = 0; n < 256; ++n)
            {
                int c = n;
                for (int k = 0; k < 8; ++k)
                    c = (c & 1) != 0 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
                _table[n] = c;
            }
        }

        CRC32() { _makeTable(); }
        void update(int c) { _crc = _table[(_crc ^ c) & 0xff] ^ (_crc >>> 8); }
        int crc() { return ~_crc; }
    }

    class CRCOutputStream extends OutputStream
    {
        OutputStream _os;
        int _cnt = 0;
        CRC32 _crc = new CRC32();
        public CRCOutputStream(OutputStream os) { _os = os; }
        public int crc() { return _crc.crc(); }
        public int cnt() { return _cnt; }

        public void write(int b) throws IOException
        {
            _cnt++;
            _crc.update(b);
            _os.write(b);
        }
    }

    class CanonicalCode
    {
        int[] symbolCodeBits, symbolValues;
        int _numSymbolsAllocated = 0;
        static final int MAX_CODE_LENGTH = 15;
    
        CanonicalCode(int[] codeLengths, int n)
        {
            symbolCodeBits = new int[n];
            symbolValues = new int[n];
            for (int codeLength = 1, nextCode = 0; codeLength <= MAX_CODE_LENGTH; codeLength++)
            {
                nextCode <<= 1;
                int startBit = 1 << codeLength;
                for (int symbol = 0; symbol < n; symbol++)
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
        byte[] _data;
        int _index = 0, _length = 0, _size;
        ByteHistory(int size) { _size = size; _data = new byte[size]; }
    
        void append(int b)
        {
            _data[_index] = (byte)b;
            _index = (_index + 1) % _size;
            if (_length < _size) _length++;
        }
    
        void copy(int dist, int len, OutputStream out) throws IOException
        {
            int readIndex = (_index - dist + _size) % _size;
            for (int i = 0; i < len; i++)
            {
                byte b = _data[readIndex];
                readIndex = (readIndex + 1) % _size;
                out.write(b);
                append(b);
            }
        }
    }
    
    class Inflater
    {
        final ByteBitInputStream _bis;
        final OutputStream output;
        final ByteHistory dictionary;
        CanonicalCode FIXED_LITERAL_LENGTH_CODE;
        CanonicalCode FIXED_DISTANCE_CODE;

        Inflater(ByteBitInputStream in, OutputStream out)
            throws IOException, DataFormatException
        {
            _bis = Objects.requireNonNull(in);
            output = Objects.requireNonNull(out);
            dictionary = new ByteHistory(32 * 1024);
            int[] llcodelens = new int[288];
            int i = 0;
            for (; i < 144; i++) llcodelens[i] = 8;
            for (; i < 256; i++) llcodelens[i] = 9;
            for (; i < 280; i++) llcodelens[i] = 7;
            for (; i < 288; i++) llcodelens[i] = 8;
            FIXED_LITERAL_LENGTH_CODE = new CanonicalCode(llcodelens, 288);
            int[] distcodelens = new int[32];
            for (i = 0; i < 32; i++) distcodelens[i] = 5;
            FIXED_DISTANCE_CODE = new CanonicalCode(distcodelens, 32);
        }

        void inflate() throws IOException, DataFormatException
        {
            for (boolean isFinal = false; !isFinal;)
            {
                isFinal = _bis.readUint(1) != 0;  // bfinal
                int type = _bis.readUint(2);  // btype
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
            int numLitLenCodes = _bis.readUint(5) + 257;  // hlit + 257
            int numDistCodes = _bis.readUint(5) + 1;      // hdist + 1
            int numCodeLenCodes = _bis.readUint(4) + 4;   // hclen + 4
            int[] codeLenCodeLen = new int[19];  // This array is filled in a strange order
            codeLenCodeLen[16] = _bis.readUint(3);
            codeLenCodeLen[17] = _bis.readUint(3);
            codeLenCodeLen[18] = _bis.readUint(3);
            codeLenCodeLen[ 0] = _bis.readUint(3);
            for (int i = 0; i < numCodeLenCodes - 4; i++)
            {
                int j = (i % 2 == 0) ? (8 + i / 2) : (7 - i / 2);
                codeLenCodeLen[j] = _bis.readUint(3);
            }
        
            CanonicalCode codeLenCode = new CanonicalCode(codeLenCodeLen, 19);
            int nCodeLens = numLitLenCodes + numDistCodes;
            int[] codeLens = new int[nCodeLens];
            for (int codeLensIndex = 0; codeLensIndex < nCodeLens; )
            {
                int sym = codeLenCode.decodeNextSymbol(_bis);
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
                        runLen = _bis.readUint(2) + 3;
                        runVal = codeLens[codeLensIndex - 1];
                    } else if (sym == 17) {
                        runLen = _bis.readUint(3) + 3;
                    }
                    else if (sym == 18)
                        runLen = _bis.readUint(7) + 11;
                    else
                        throw new AssertionError("Symbol out of range");
                    int end = codeLensIndex + runLen;
                    for (int i = codeLensIndex; i < end; ++i)
                        codeLens[i] = runVal;
                    codeLensIndex = end;
                }
            }
            int[] litLenCodeLen = new int[numLitLenCodes];
            for (int i = 0; i < numLitLenCodes; i++)
                litLenCodeLen[i] = codeLens[i];
            CanonicalCode litLenCode = new CanonicalCode(litLenCodeLen, numLitLenCodes);
            int nDistCodeLen = nCodeLens - numLitLenCodes;
            int[] distCodeLen = new int[nDistCodeLen];
            for (int i = 0, j = numLitLenCodes; j < nCodeLens; i++, j++)
                distCodeLen[i] = codeLens[j];
            CanonicalCode distCode;
            if (nDistCodeLen == 1 && distCodeLen[0] == 0)
            {
                //distCode = null;
                distCode = new CanonicalCode(distCodeLen, nDistCodeLen);  //bogus CanonicalCode
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
                distCode = new CanonicalCode(distCodeLen, nDistCodeLen);
            }
        
            return new CanonicalCode[]{litLenCode, distCode};
        }
    
        void decompressUncompressedBlock() throws IOException, DataFormatException
        {
            while (_bis.getBitPosition() != 0)
                _bis.readUint(1);
            int  len = _bis.readUint(16);
            int nlen = _bis.readUint(16);
            if ((len ^ 0xFFFF) != nlen)
                throw new DataFormatException("Invalid length in uncompressed block");
            for (int i = 0; i < len; i++)
            {
                int b = _bis.readUint(8);  // Byte is aligned
                output.write(b);
                dictionary.append(b);
            }
        }
    
        void decompressHuffmanBlock(CanonicalCode litLenCode, CanonicalCode distCode)
            throws IOException, DataFormatException
        {
            for (int sym; (sym = litLenCode.decodeNextSymbol(_bis)) != 256;)
            {
                if (sym < 256)
                {
                    output.write(sym);
                    dictionary.append(sym);
                }
                else
                {
                    int run = decodeRunLength(sym);
                    int distSym = distCode.decodeNextSymbol(_bis);
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
                return (((sym - 265) % 4 + 4) << numExtraBits) + 3 + _bis.readUint(numExtraBits);
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
                return ((sym % 2 + 2) << numExtraBits) + 1 + _bis.readUint(numExtraBits);
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
            Inflater dec = new Inflater(new ByteBitInputStream(in), output);
            dec.inflate();
            os.flush();
            int crc  = readLittleEndianInt32(in);
            int size = readLittleEndianInt32(in);
            msgOut.println("size: " + size + " " + output.cnt());
            msgOut.println(String.format("CRC32: 0x%08X 0x%08X", crc, output.crc()));
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
        String msg = new Gzcat().submain(new File(args[0]), System.out, System.err);

        if (msg != null)
        {
            System.err.println(msg);
            System.exit(1);
        }
    }
}
