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
        static final int BLOCK_HEADER_MARKER_1 = 0x314159;
        static final int BLOCK_HEADER_MARKER_2 = 0x265359;
        static final int HUFFMAN_GROUP_RUN_LENGTH = 50;
        static final int HUFFMAN_MAXIMUM_ALPHABET_SIZE = 258;
        static final int HUFFMAN_ENCODE_MAXIMUM_CODE_LENGTH = 20;
        static final int HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH = 23;
        static final int HUFFMAN_MINIMUM_TABLES = 2;
        static final int HUFFMAN_MAXIMUM_TABLES = 6;
        static final int HUFFMAN_MAXIMUM_SELECTORS = (900000 / HUFFMAN_GROUP_RUN_LENGTH) + 1;
        static final int HUFFMAN_SYMBOL_RUNA = 0;
        static final int HUFFMAN_SYMBOL_RUNB = 1;
        static final int STREAM_START_MARKER_1 = 0x425a;
        static final int STREAM_START_MARKER_2 = 0x68;
    }

    public class BZip2HuffmanStageDecoder
    {
        final BZip2BitInputStream bitInputStream;
        final byte[] selectors;
        final int[] minimumLengths = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES];

        final int[][] codeBases = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                            BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 2];

        final int[][] codeLimits = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                                BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 1];

        final int[][] codeSymbols = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                                BZip2Constants.HUFFMAN_MAXIMUM_ALPHABET_SIZE];

        int currentTable;
        int groupIndex = -1;
        int groupPosition = -1;

        void _createHuffmanDecodingTables(final int alphabetSize,
            final byte[][] tableCodeLengths)
        {
            for (int table = 0; table < tableCodeLengths.length; table++)
            {
                final int[] tableBases = this.codeBases[table];
                final int[] tableLimits = this.codeLimits[table];
                final int[] tableSymbols = this.codeSymbols[table];

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

        public int nextSymbol() throws IOException
        {
            final BZip2BitInputStream bitInputStream = this.bitInputStream;

            // Move to next group selector if required
            if (((++this.groupPosition % BZip2Constants.HUFFMAN_GROUP_RUN_LENGTH) == 0))
            {
            this.groupIndex++;
            if (this.groupIndex == this.selectors.length) {
                throw new BZip2Exception ("Error decoding BZip2 block");
            }
            this.currentTable = this.selectors[this.groupIndex] & 0xff;
            }

            final int currentTable = this.currentTable;
            final int[] tableLimits = this.codeLimits[currentTable];
            int codeLength = this.minimumLengths[currentTable];

            int codeBits = bitInputStream.readBits (codeLength);
            for (; codeLength <= BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH; codeLength++)
            {
                if (codeBits <= tableLimits[codeLength])
                {
                    return this.codeSymbols[currentTable][
                        codeBits - this.codeBases[currentTable][codeLength]];
                }
                codeBits = (codeBits << 1) | bitInputStream.readBits (1);
            }

            // A valid code was not recognised
            throw new BZip2Exception ("Error decoding BZip2 block");

        }

        public BZip2HuffmanStageDecoder (final BZip2BitInputStream bitInputStream,
            final int alphabetSize, final byte[][] tableCodeLengths, final byte[] selectors)
        {
            this.bitInputStream = bitInputStream;
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

        public void updateCRC(final int value, int count)
        {
            while (count-- > 0)
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
        private final byte[] _mtf = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
            46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
            68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
            90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
            109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
            126, 127, (byte)128, (byte)129, (byte)130, (byte)131, (byte)132, (byte)133, (byte)134,
            (byte)135, (byte)136, (byte)137, (byte)138, (byte)139, (byte)140, (byte)141, (byte)142,
            (byte)143, (byte)144, (byte)145, (byte)146, (byte)147, (byte)148, (byte)149, (byte)150,
            (byte)151, (byte)152, (byte)153, (byte)154, (byte)155, (byte)156, (byte)157, (byte)158,
            (byte)159, (byte)160, (byte)161, (byte)162, (byte)163, (byte)164, (byte)165, (byte)166,
            (byte)167, (byte)168, (byte)169, (byte)170, (byte)171, (byte)172, (byte)173, (byte)174,
            (byte)175, (byte)176, (byte)177, (byte)178, (byte)179, (byte)180, (byte)181, (byte)182,
            (byte)183, (byte)184, (byte)185, (byte)186, (byte)187, (byte)188, (byte)189, (byte)190,
            (byte)191, (byte)192, (byte)193, (byte)194, (byte)195, (byte)196, (byte)197, (byte)198,
            (byte)199, (byte)200, (byte)201, (byte)202, (byte)203, (byte)204, (byte)205, (byte)206,
            (byte)207, (byte)208, (byte)209, (byte)210, (byte)211, (byte)212, (byte)213, (byte)214,
            (byte)215, (byte)216, (byte)217, (byte)218, (byte)219, (byte)220, (byte)221, (byte)222,
            (byte)223, (byte)224, (byte)225, (byte)226, (byte)227, (byte)228, (byte)229, (byte)230,
            (byte)231, (byte)232, (byte)233, (byte)234, (byte)235, (byte)236, (byte)237, (byte)238,
            (byte)239, (byte)240, (byte)241, (byte)242, (byte)243, (byte)244, (byte)245, (byte)246,
            (byte)247, (byte)248, (byte)249, (byte)250, (byte)251, (byte)252, (byte)253, (byte)254,
            (byte)255
        };

        public int valueToFront (final byte value)
        {
            final byte[] mtf = _mtf;
            int index = 0;
            byte temp = mtf[0];
            if (value != temp)
            {
                mtf[0] = value;
                while (value != temp)
                {
                    index++;
                    final byte temp2 = temp;
                    temp = mtf[index];
                    mtf[index] = temp2;
                }
            }

            return index;
        }

        public byte indexToFront (final int index)
        {
            final byte[] mtf = _mtf;
            final byte value = mtf[index];
            System.arraycopy (mtf, 0, mtf, 1, index);
            mtf[0] = value;
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
            final BZip2BitInputStream bitInputStream = this.bitInputStream;
            final byte[] huffmanSymbolMap = this.huffmanSymbolMap;

            final byte[][] tableCodeLengths = new byte[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                                               BZip2Constants.HUFFMAN_MAXIMUM_ALPHABET_SIZE];

            int huffmanUsedRanges = bitInputStream.readBits (16);
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
            if (
                   (totalTables < BZip2Constants.HUFFMAN_MINIMUM_TABLES)
                || (totalTables > BZip2Constants.HUFFMAN_MAXIMUM_TABLES)
                || (totalSelectors < 1)
                || (totalSelectors > BZip2Constants.HUFFMAN_MAXIMUM_SELECTORS)
            )
            {
                throw new BZip2Exception ("BZip2 block Huffman tables invalid");
            }

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

        private void initialiseInverseBWT (final int bwtStartPointer) throws IOException
        {
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

        public int read (final byte[] destination, int offset, final int length)
        {
            int i;
            for (i = 0; i < length; i++, offset++)
            {
                int decoded = read();
                if (decoded == -1) {
                    return (i == 0) ? -1 : i;
                }
                destination[offset] = (byte)decoded;
            }
            return i;
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
            initialiseInverseBWT (bwtStartPointer);
        }
    }

    void decompress(InputStream is, OutputStream os) throws IOException
    {
        BZip2BitInputStream bis = new BZip2BitInputStream(is);

        int smarker1 = bis.readBits(16);
        int smarker2 = bis.readBits(8);
        int blockSize = (bis.readBits(8) - '0');
        int streamBlockSize = blockSize * 100000;
        int _streamCRC = 0;
        BZip2BlockDecompressor _blockDecompressor = null;

        while (true)
        {
            if (_blockDecompressor != null)
            {
                int blockCRC = _blockDecompressor.checkCRC();
                _streamCRC = ((_streamCRC << 1) | (_streamCRC >>> 31)) ^ blockCRC;
            }
 
            int marker1 = bis.readBits(24);
            int marker2 = bis.readBits(24);
 
            if (marker1 == 0x314159 && marker2 == 0x265359)
            {
                _blockDecompressor = new BZip2BlockDecompressor(bis, streamBlockSize);
                
                int c;

                while ((c = _blockDecompressor.read()) != -1)
                    os.write(c);

                continue;
            }
 
            if (marker1 == 0x177245 && marker2 == 0x385090)
            {
                os.flush();
                int crc = bis.readInteger();
                System.err.println(String.format("0x%08X 0x%08X", _streamCRC, crc));
 
                if (crc != _streamCRC)
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


