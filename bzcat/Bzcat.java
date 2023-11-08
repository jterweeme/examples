//van github/volueinsight/bzip2

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

public class Bzcat
{
    class BZip2BitInputStream
    {
        private final InputStream inputStream;
        private int bitBuffer;
        private int bitCount;

        public boolean readBoolean() throws IOException
        {
            int bitBuffer = this.bitBuffer;
            int bitCount = this.bitCount;

            if (bitCount > 0) {
                bitCount--;
            }
            else
            {
                int byteRead = this.inputStream.read();
                
                if (byteRead < 0) {
                    throw new BZip2Exception ("Insufficient data");
                }

                bitBuffer = (bitBuffer << 8) | byteRead;
                bitCount += 7;
                this.bitBuffer = bitBuffer;
            }

            this.bitCount = bitCount;
            return ((bitBuffer & (1 << bitCount))) != 0;
        }

        public int readUnary() throws IOException
        {
            int bitBuffer = this.bitBuffer;
            int bitCount = this.bitCount;
            int unaryCount = 0;
    
            for (;;) {
                if (bitCount > 0) {
                    bitCount--;
                } else  {
                    int byteRead = this.inputStream.read();
    
                    if (byteRead < 0) {
                        throw new BZip2Exception ("Insufficient data");
                    }
    
                    bitBuffer = (bitBuffer << 8) | byteRead;
                    bitCount += 7;
                }
    
                if (((bitBuffer & (1 << bitCount))) == 0) {
                    this.bitBuffer = bitBuffer;
                    this.bitCount = bitCount;
                    return unaryCount;
                }
                unaryCount++;
            }
        }

        public int readBits (final int count) throws IOException
        {
            int bitBuffer = this.bitBuffer;
            int bitCount = this.bitCount;
    
            if (bitCount < count) {
                while (bitCount < count) {
                    int byteRead = this.inputStream.read();
    
                    if (byteRead < 0) {
                        throw new BZip2Exception ("Insufficient data");
                    }
    
                    bitBuffer = (bitBuffer << 8) | byteRead;
                    bitCount += 8;
                }
    
                this.bitBuffer = bitBuffer;
            }
    
            bitCount -= count;
            this.bitCount = bitCount;
            return (bitBuffer >>> bitCount) & ((1 << count) - 1);
        }

        public int readInteger() throws IOException {
            return (readBits (16) << 16) | (readBits (16));
        }

        public BZip2BitInputStream (final InputStream inputStream)
        {
            this.inputStream = inputStream;
        }
    }

    interface BZip2Constants
    {
        static final int HUFFMAN_ENCODE_MAXIMUM_CODE_LENGTH = 20;
        static final int HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH = 23;
        static final int HUFFMAN_SYMBOL_RUNA = 0;
        static final int HUFFMAN_SYMBOL_RUNB = 1;
    }

    public class BZip2HuffmanStageDecoder
    {
        final BZip2BitInputStream _bis;
        final byte[] selectors;
        final int[] minimumLengths = new int[6];
        final int[][] _codeBases = new int[6][25];
        final int[][] codeLimits = new int[6][24];
        final int[][] _codeSymbols = new int[6][258];
        int currentTable;
        int groupIndex = -1;
        int groupPosition = -1;

        public int nextSymbol() throws IOException
        {
            // Move to next group selector if required
            if (((++this.groupPosition % 50) == 0))
            {
                this.groupIndex++;
                if (this.groupIndex == this.selectors.length)
                    throw new BZip2Exception ("Error decoding BZip2 block");
                
                this.currentTable = this.selectors[this.groupIndex] & 0xff;
            }

            final int currentTable = this.currentTable;
            final int[] tableLimits = this.codeLimits[currentTable];
            int codeLength = this.minimumLengths[currentTable];

            int codeBits = _bis.readBits(codeLength);
            for (; codeLength <= BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH; codeLength++)
            {
                if (codeBits <= tableLimits[codeLength])
                {
                    return _codeSymbols[currentTable][
                            codeBits - _codeBases[currentTable][codeLength]];
                }
                codeBits = (codeBits << 1) | _bis.readBits (1);
            }

            // A valid code was not recognised
            throw new BZip2Exception ("Error decoding BZip2 block");

        }

        void _createHuffmanDecodingTables(final int alphabetSize,
            final byte[][] tableCodeLengths)
        {
            for (int table = 0; table < tableCodeLengths.length; table++)
            {
                final int[] tableBases = _codeBases[table];
                final int[] tableLimits = this.codeLimits[table];
                final int[] tableSymbols = _codeSymbols[table];

                final byte[] codeLengths = tableCodeLengths[table];
                int minimumLength = BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH;
                int maximumLength = 0;

                // Find the minimum and maximum code length for the table
                for (int i = 0; i < alphabetSize; i++) {
                    maximumLength = Math.max (codeLengths[i], maximumLength);
                    minimumLength = Math.min (codeLengths[i], minimumLength);
                }

                this.minimumLengths[table] = minimumLength;

                // Calculate the first output symbol for each code length
                for (int i = 0; i < alphabetSize; i++)
                    tableBases[codeLengths[i] + 1]++;
                
                for (int i = 1; i < BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 2; i++)
                    tableBases[i] += tableBases[i - 1];

                // Calculate the first and last Huffman code for each code length (codes at a given
                // length are sequential in value)
                int code = 0;
                for (int i = minimumLength; i <= maximumLength; i++)
                {
                    int base = code;
                    code += tableBases[i + 1] - tableBases[i];
                    tableBases[i] = base - tableBases[i];
                    tableLimits[i] = code - 1;
                    code <<= 1;
                }
                // Populate the mapping from canonical code index to output symbol
                int codeIndex = 0;
                for (int bitLength = minimumLength; bitLength <= maximumLength; bitLength++)
                    for (int symbol = 0; symbol < alphabetSize; symbol++)
                        if (codeLengths[symbol] == bitLength)
                            tableSymbols[codeIndex++] = symbol;

            }
        }



        public BZip2HuffmanStageDecoder (final BZip2BitInputStream bitInputStream,
            final int alphabetSize, final byte[][] tableCodeLengths, final byte[] selectors)
        {
            _bis = bitInputStream;
            this.selectors = selectors;
            this.currentTable = this.selectors[0];
            _createHuffmanDecodingTables (alphabetSize, tableCodeLengths);
        }
    }

    public final class CRC32
    {
        private int _table[] = new int[256];
        private int _crc = 0xffffffff;
        public int getCRC() { return ~_crc; }

        CRC32()
        {
            //https://github.com/gcc-mirror/gcc/blob/master/libiberty/crc32.c
            for (int i = 0, j, c; i < 256; i++)
            {
                for (c = i << 24, j = 8; j > 0; --j)
                    c = (c & 0x80000000) != 0 ? (c << 1) ^ 0x04c11db7 : (c << 1);
                _table[i] = c;
            }
        }

        public void updateCRC(final int value)
        {
            _crc = (_crc << 8) ^ _table[((_crc >> 24) ^ value) & 0xff];
        }
    }

    public class BZip2Exception extends IOException
    {
        private static final long serialVersionUID = -8931219115669559570L;
        public BZip2Exception (String reason) {
            super (reason);
        }
    }

    public class MoveToFront
    {
        private byte[] _mtf = new byte[256];

        public MoveToFront()
        {
            for (int i = 0; i < 256; ++i)
                _mtf[i] = (byte)i;
        }

        public byte indexToFront (final int index)
        {
            final byte value = _mtf[index];
            System.arraycopy (_mtf, 0, _mtf, 1, index);
            _mtf[0] = value;
            return value;
        }
    }

    public class BZip2BlockDecompressor
    {
        final BZip2BitInputStream bitInputStream;
        final CRC32 _crc = new CRC32();
        final int _blockCRC;
        int huffmanEndOfBlockSymbol;
        final byte[] huffmanSymbolMap = new byte[256];
        final int[] bwtByteCounts = new int[256];
        byte[] bwtBlock;
        int[] bwtMergedPointers;
        int bwtCurrentMergedPointer;
        int bwtBlockLength;
        int bwtBytesDecoded;
        int rleLastDecodedByte = -1;
        int rleAccumulator;
        int _rleRepeat;

        private BZip2HuffmanStageDecoder readHuffmanTables() throws IOException
        {
            final byte[] huffmanSymbolMap = this.huffmanSymbolMap;
            final byte[][] tableCodeLengths = new byte[6][258];
            int huffmanUsedRanges = bitInputStream.readBits(16);
            int huffmanSymbolCount = 0;

            for (int i = 0; i < 16; i++)
                if ((huffmanUsedRanges & ((1 << 15) >>> i)) != 0)
                    for (int j = 0, k = i << 4; j < 16; j++, k++)
                        if (bitInputStream.readBoolean())
                            huffmanSymbolMap[huffmanSymbolCount++] = (byte)k;
            
            int endOfBlockSymbol = huffmanSymbolCount + 1;
            this.huffmanEndOfBlockSymbol = endOfBlockSymbol;

            /* Read total number of tables and selectors*/
            final int totalTables = bitInputStream.readBits (3);
            final int totalSelectors = bitInputStream.readBits (15);

            /* Read and decode MTFed Huffman selector list */
            final MoveToFront tableMTF = new MoveToFront();
            final byte[] selectors = new byte[totalSelectors];
            for (int selector = 0; selector < totalSelectors; selector++)
                selectors[selector] = tableMTF.indexToFront (bitInputStream.readUnary());

            /* Read the Canonical Huffman code lengths for each table */
            for (int table = 0; table < totalTables; table++)
            {
                int currentLength = bitInputStream.readBits (5);
                for (int i = 0; i <= endOfBlockSymbol; i++)
                {
                    while (bitInputStream.readBoolean())
                        currentLength += bitInputStream.readBoolean() ? -1 : 1;
                    tableCodeLengths[table][i] = (byte)currentLength;
                }
            }

            return new BZip2HuffmanStageDecoder(bitInputStream, endOfBlockSymbol + 1,
                                                tableCodeLengths, selectors);
        }

        private void decodeHuffmanData (final BZip2HuffmanStageDecoder huffmanDecoder)
            throws IOException
        {
            final byte[] bwtBlock = this.bwtBlock;
            final byte[] huffmanSymbolMap = this.huffmanSymbolMap;
            final int streamBlockSize = this.bwtBlock.length;
            final int huffmanEndOfBlockSymbol = this.huffmanEndOfBlockSymbol;
            final int[] bwtByteCounts = this.bwtByteCounts;
            final MoveToFront symbolMTF = new MoveToFront();
            int bwtBlockLength = 0;
            int repeatCount = 0;
            int repeatIncrement = 1;
            int mtfValue = 0;

            for (;;)
            {
                final int nextSymbol = huffmanDecoder.nextSymbol();

                if (nextSymbol == BZip2Constants.HUFFMAN_SYMBOL_RUNA)
                {
                    repeatCount += repeatIncrement;
                    repeatIncrement <<= 1;
                    continue;
                }
                if (nextSymbol == BZip2Constants.HUFFMAN_SYMBOL_RUNB)
                {
                    repeatCount += repeatIncrement << 1;
                    repeatIncrement <<= 1;
                    continue;
                }
                
                if (repeatCount > 0)
                {
                    if (bwtBlockLength + repeatCount > streamBlockSize)
                        throw new BZip2Exception ("BZip2 block exceeds declared block size");
                
                    final byte nextByte = huffmanSymbolMap[mtfValue];
                    bwtByteCounts[nextByte & 0xff] += repeatCount;
                    while (--repeatCount >= 0)
                        bwtBlock[bwtBlockLength++] = nextByte;

                    repeatCount = 0;
                    repeatIncrement = 1;
                }

                if (nextSymbol == huffmanEndOfBlockSymbol)
                    break;

                if (bwtBlockLength >= streamBlockSize)
                    throw new BZip2Exception ("BZip2 block exceeds declared block size");

                mtfValue = symbolMTF.indexToFront (nextSymbol - 1) & 0xff;
                final byte nextByte = huffmanSymbolMap[mtfValue];
                bwtByteCounts[nextByte & 0xff]++;
                bwtBlock[bwtBlockLength++] = nextByte;
            
            }

            this.bwtBlockLength = bwtBlockLength;
        }

        private int decodeNextBWTByte()
        {
            int mergedPointer = this.bwtCurrentMergedPointer;
            int nextDecodedByte =  mergedPointer & 0xff;
            this.bwtCurrentMergedPointer = this.bwtMergedPointers[mergedPointer >>> 8];
            this.bwtBytesDecoded++;
            return nextDecodedByte;
        }

        public int read()
        {
            while (_rleRepeat < 1)
            {
                if (this.bwtBytesDecoded == this.bwtBlockLength)
                    return -1;

                int nextByte = decodeNextBWTByte();

                if (nextByte != this.rleLastDecodedByte)
                {
                    // New byte, restart accumulation
                    this.rleLastDecodedByte = nextByte;
                    _rleRepeat = 1;
                    this.rleAccumulator = 1;
                    _crc.updateCRC (nextByte);
                }
                else if (++this.rleAccumulator == 4)
                {
                    // Accumulation complete, start repetition
                    _rleRepeat = decodeNextBWTByte() + 1;
                    this.rleAccumulator = 0;

                    for (int i = 0; i < _rleRepeat; ++i)
                        _crc.updateCRC(nextByte);
                    //_crc.updateCRC (nextByte, _rleRepeat);
                }
                else
                {
                    _rleRepeat = 1;
                    _crc.updateCRC(nextByte);
                }
            }

            _rleRepeat--;
            return this.rleLastDecodedByte;
        }

        public int checkCRC() throws IOException
        {
            if (_blockCRC != _crc.getCRC())
                throw new BZip2Exception ("BZip2 block CRC error");

            return _crc.getCRC();
        }

        public BZip2BlockDecompressor(final BZip2BitInputStream bitInputStream,
                            final int blockSize) throws IOException
        {
            this.bitInputStream = bitInputStream;
            this.bwtBlock = new byte[blockSize];

            final int bwtStartPointer;

            // Read block header
            _blockCRC = this.bitInputStream.readInteger();
            boolean blockRandomised = this.bitInputStream.readBoolean();

            if (blockRandomised)
                throw new BZip2Exception("Randomised blocks not supported.");

            bwtStartPointer = this.bitInputStream.readBits (24);

            // Read block data and decode through to the Inverse Burrows Wheeler Transform stage
            BZip2HuffmanStageDecoder huffmanDecoder = readHuffmanTables();
            decodeHuffmanData (huffmanDecoder);
            //initialiseInverseBWT (bwtStartPointer);
            final byte[] bwtBlock  = this.bwtBlock;
            final int[] bwtMergedPointers = new int[this.bwtBlockLength];
            final int[] characterBase = new int[256];

            if ((bwtStartPointer < 0) || (bwtStartPointer >= this.bwtBlockLength))
                throw new BZip2Exception ("BZip2 start pointer invalid");

            // Cumulatise character counts
            System.arraycopy (this.bwtByteCounts, 0, characterBase, 1, 255);
            for (int i = 2; i <= 255; i++)
                characterBase[i] += characterBase[i - 1];

            for (int i = 0; i < this.bwtBlockLength; i++)
            {
                int value = bwtBlock[i] & 0xff;
                bwtMergedPointers[characterBase[value]++] = (i << 8) + value;
            }

            this.bwtBlock = null;
            this.bwtMergedPointers = bwtMergedPointers;
            this.bwtCurrentMergedPointer = bwtMergedPointers[bwtStartPointer];
        }
    }

    void decompress(InputStream is, OutputStream os) throws IOException
    {
        BZip2BitInputStream bis = new BZip2BitInputStream(is);

        int smarker1 = bis.readBits(16);
        int smarker2 = bis.readBits(8);
        int blockSize = (bis.readBits(8) - '0');
        int streamBlockSize = blockSize * 100000;
        int streamCRC = 0;

        while (true)
        {
            int marker1 = bis.readBits(24);
            int marker2 = bis.readBits(24);
 
            if (marker1 == 0x314159 && marker2 == 0x265359)
            {
                BZip2BlockDecompressor block = new BZip2BlockDecompressor(bis, streamBlockSize);
                
                for (int c; (c = block.read()) != -1;)
                    os.write(c);

                int blockCRC = block.checkCRC();
                streamCRC = ((streamCRC << 1) | (streamCRC >>> 31)) ^ blockCRC;
                continue;
            }
 
            if (marker1 == 0x177245 && marker2 == 0x385090)
            {
                os.flush();
                int crc = bis.readInteger();
                System.err.println(String.format("0x%08X 0x%08X", streamCRC, crc));
 
                if (crc != streamCRC)
                    throw new BZip2Exception ("BZip2 stream CRC error");
                 
                break;
            }
 
            throw new BZip2Exception ("BZip2 stream format error");
        }
    }

    void submain(String[] args) throws IOException
    {
        InputStream is = System.in;
        File inputFile;

        if (args.length == 1)
        {
            inputFile = new File(args[0]);
            if (!inputFile.exists() || !inputFile.canRead() || !args[0].endsWith(".bz2"))
            {
                System.err.println ("Cannot read file " + inputFile.getPath());
                System.exit (1);
            }
            is = new BufferedInputStream(new FileInputStream(inputFile));
        }
        decompress(is, System.out);
    }

    public static void main (String[] args) throws IOException
    {
        new Bzcat().submain(args);
    }
}


