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

    interface BZip2Constants {

    /**
     * First three bytes of the block header marker
     */
    static final int BLOCK_HEADER_MARKER_1 = 0x314159;

    /**
     * Last three bytes of the block header marker
     */
    static final int BLOCK_HEADER_MARKER_2 = 0x265359;

    /**
     * Number of symbols decoded after which a new Huffman table is selected
     */
    static final int HUFFMAN_GROUP_RUN_LENGTH = 50;

    /**
     * Maximum possible Huffman alphabet size
     */
    static final int HUFFMAN_MAXIMUM_ALPHABET_SIZE = 258;

    /**
     * The longest Huffman code length created by the encoder
     */
    static final int HUFFMAN_ENCODE_MAXIMUM_CODE_LENGTH = 20;

    /**
     * The longest Huffman code length accepted by the decoder
     */
    static final int HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH = 23;

    /**
     * Minimum number of alternative Huffman tables
     */
    static final int HUFFMAN_MINIMUM_TABLES = 2;
static final int HUFFMAN_MAXIMUM_TABLES = 6;

    /**
     * Maximum possible number of Huffman table selectors
     */
    static final int HUFFMAN_MAXIMUM_SELECTORS = (900000 / HUFFMAN_GROUP_RUN_LENGTH) + 1;

    /**
     * Huffman symbol used for run-length encoding
     */
    static final int HUFFMAN_SYMBOL_RUNA = 0;

    /**
     * Huffman symbol used for run-length encoding
     */
    static final int HUFFMAN_SYMBOL_RUNB = 1;

    /**
     * First three bytes of the end of stream marker
     */
    static final int STREAM_END_MARKER_1 = 0x177245;

    /**
     * Last three bytes of the end of stream marker
     */
    static final int STREAM_END_MARKER_2 = 0x385090;

    /**
     * 'B' 'Z' that marks the start of a BZip2 stream
     */
    static final int STREAM_START_MARKER_1 = 0x425a;

    /**
     * 'h' that distinguishes BZip from BZip2
     */
    static final int STREAM_START_MARKER_2 = 0x68;
    }

    public class BZip2HuffmanStageDecoder
    {
        private final BZip2BitInputStream bitInputStream;
        private final byte[] selectors;
        private final int[] minimumLengths = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES];

        private final int[][] codeBases = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                            BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 2];

        private final int[][] codeLimits = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                                BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 1];

        private final int[][] codeSymbols = new int[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                                BZip2Constants.HUFFMAN_MAXIMUM_ALPHABET_SIZE];

        private int currentTable;
        private int groupIndex = -1;
        private int groupPosition = -1;

        private void createHuffmanDecodingTables (final int alphabetSize,
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
                for (int i = 0; i < alphabetSize; i++) {
                    tableBases[codeLengths[i] + 1]++;
                }
                for (int i = 1; i < BZip2Constants.HUFFMAN_DECODE_MAXIMUM_CODE_LENGTH + 2; i++) {
                    tableBases[i] += tableBases[i - 1];
                }

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
                {
                    for (int symbol = 0; symbol < alphabetSize; symbol++)
                    {
                        if (codeLengths[symbol] == bitLength)
                        {
                            tableSymbols[codeIndex++] = symbol;
                        }
                    }
                }

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
            createHuffmanDecodingTables (alphabetSize, tableCodeLengths);
        }
    }

    public final class CRC32
    {
        private static final int crc32Lookup[] = {
        0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
        0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd,
        0x4c11db70,0x48d0c6c7,0x4593e01e,0x4152fda9,0x5f15adac,0x5bd4b01b,0x569796c2,0x52568b75,
        0x6a1936c8,0x6ed82b7f,0x639b0da6,0x675a1011,0x791d4014,0x7ddc5da3,0x709f7b7a,0x745e66cd,
        0x9823b6e0,0x9ce2ab57,0x91a18d8e,0x95609039,0x8b27c03c,0x8fe6dd8b,0x82a5fb52,0x8664e6e5,
        0xbe2b5b58,0xbaea46ef,0xb7a96036,0xb3687d81,0xad2f2d84,0xa9ee3033,0xa4ad16ea,0xa06c0b5d,
        0xd4326d90,0xd0f37027,0xddb056fe,0xd9714b49,0xc7361b4c,0xc3f706fb,0xceb42022,0xca753d95,
        0xf23a8028,0xf6fb9d9f,0xfbb8bb46,0xff79a6f1,0xe13ef6f4,0xe5ffeb43,0xe8bccd9a,0xec7dd02d,
        0x34867077,0x30476dc0,0x3d044b19,0x39c556ae,0x278206ab,0x23431b1c,0x2e003dc5,0x2ac12072,
        0x128e9dcf,0x164f8078,0x1b0ca6a1,0x1fcdbb16,0x018aeb13,0x054bf6a4,0x0808d07d,0x0cc9cdca,
        0x7897ab07,0x7c56b6b0,0x71159069,0x75d48dde,0x6b93dddb,0x6f52c06c,0x6211e6b5,0x66d0fb02,
        0x5e9f46bf,0x5a5e5b08,0x571d7dd1,0x53dc6066,0x4d9b3063,0x495a2dd4,0x44190b0d,0x40d816ba,
        0xaca5c697,0xa864db20,0xa527fdf9,0xa1e6e04e,0xbfa1b04b,0xbb60adfc,0xb6238b25,0xb2e29692,
        0x8aad2b2f,0x8e6c3698,0x832f1041,0x87ee0df6,0x99a95df3,0x9d684044,0x902b669d,0x94ea7b2a,
        0xe0b41de7,0xe4750050,0xe9362689,0xedf73b3e,0xf3b06b3b,0xf771768c,0xfa325055,0xfef34de2,
        0xc6bcf05f,0xc27dede8,0xcf3ecb31,0xcbffd686,0xd5b88683,0xd1799b34,0xdc3abded,0xd8fba05a,
        0x690ce0ee,0x6dcdfd59,0x608edb80,0x644fc637,0x7a089632,0x7ec98b85,0x738aad5c,0x774bb0eb,
        0x4f040d56,0x4bc510e1,0x46863638,0x42472b8f,0x5c007b8a,0x58c1663d,0x558240e4,0x51435d53,
        0x251d3b9e,0x21dc2629,0x2c9f00f0,0x285e1d47,0x36194d42,0x32d850f5,0x3f9b762c,0x3b5a6b9b,
        0x0315d626,0x07d4cb91,0x0a97ed48,0x0e56f0ff,0x1011a0fa,0x14d0bd4d,0x19939b94,0x1d528623,
        0xf12f560e,0xf5ee4bb9,0xf8ad6d60,0xfc6c70d7,0xe22b20d2,0xe6ea3d65,0xeba91bbc,0xef68060b,
        0xd727bbb6,0xd3e6a601,0xdea580d8,0xda649d6f,0xc423cd6a,0xc0e2d0dd,0xcda1f604,0xc960ebb3,
        0xbd3e8d7e,0xb9ff90c9,0xb4bcb610,0xb07daba7,0xae3afba2,0xaafbe615,0xa7b8c0cc,0xa379dd7b,
        0x9b3660c6,0x9ff77d71,0x92b45ba8,0x9675461f,0x8832161a,0x8cf30bad,0x81b02d74,0x857130c3,
        0x5d8a9099,0x594b8d2e,0x5408abf7,0x50c9b640,0x4e8ee645,0x4a4ffbf2,0x470cdd2b,0x43cdc09c,
        0x7b827d21,0x7f436096,0x7200464f,0x76c15bf8,0x68860bfd,0x6c47164a,0x61043093,0x65c52d24,
        0x119b4be9,0x155a565e,0x18197087,0x1cd86d30,0x029f3d35,0x065e2082,0x0b1d065b,0x0fdc1bec,
        0x3793a651,0x3352bbe6,0x3e119d3f,0x3ad08088,0x2497d08d,0x2056cd3a,0x2d15ebe3,0x29d4f654,
        0xc5a92679,0xc1683bce,0xcc2b1d17,0xc8ea00a0,0xd6ad50a5,0xd26c4d12,0xdf2f6bcb,0xdbee767c,
        0xe3a1cbc1,0xe760d676,0xea23f0af,0xeee2ed18,0xf0a5bd1d,0xf464a0aa,0xf9278673,0xfde69bc4,
        0x89b8fd09,0x8d79e0be,0x803ac667,0x84fbdbd0,0x9abc8bd5,0x9e7d9662,0x933eb0bb,0x97ffad0c,
        0xafb010b1,0xab710d06,0xa6322bdf,0xa2f33668,0xbcb4666d,0xb8757bda,0xb5365d03,0xb1f740b4
        };

        private int crc = 0xffffffff;

        public int getCRC()
        {
            return ~this.crc;
        }

        public void updateCRC (final int value)
        {
            final int crc = this.crc;
            this.crc = (crc << 8) ^ crc32Lookup[((crc >> 24) ^ value) & 0xff];
        }

        public void updateCRC (final int value, int count)
        {
            int crc = this.crc;

            while (count-- > 0)
                crc = (crc << 8) ^ crc32Lookup[((crc >> 24) ^ value) & 0xff];

            this.crc = crc;
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
        private final byte[] mtf = {
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

        final byte[] mtf = this.mtf;

        int index = 0;
        byte temp = mtf[0];
        if (value != temp) {
            mtf[0] = value;
            while (value != temp) {
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

        final byte[] mtf = this.mtf;

        final byte value = mtf[index];
        System.arraycopy (mtf, 0, mtf, 1, index);
        mtf[0] = value;

        return value;

        }

    }

    public class BZip2BlockDecompressor
    {
        private static final int[] RNUMS = {
            619, 720, 127, 481, 931, 816, 813, 233, 566, 247, 985, 724, 205, 454, 863, 491,
            741, 242, 949, 214, 733, 859, 335, 708, 621, 574, 73, 654, 730, 472, 419, 436,
            278, 496, 867, 210, 399, 680, 480, 51, 878, 465, 811, 169, 869, 675, 611, 697,
            867, 561, 862, 687, 507, 283, 482, 129, 807, 591, 733, 623, 150, 238, 59, 379,
            684, 877, 625, 169, 643, 105, 170, 607, 520, 932, 727, 476, 693, 425, 174, 647,
            73, 122, 335, 530, 442, 853, 695, 249, 445, 515, 909, 545, 703, 919, 874, 474,
            882, 500, 594, 612, 641, 801, 220, 162, 819, 984, 589, 513, 495, 799, 161, 604,
            958, 533, 221, 400, 386, 867, 600, 782, 382, 596, 414, 171, 516, 375, 682, 485,
            911, 276, 98, 553, 163, 354, 666, 933, 424, 341, 533, 870, 227, 730, 475, 186,
            263, 647, 537, 686, 600, 224, 469, 68, 770, 919, 190, 373, 294, 822, 808, 206,
            184, 943, 795, 384, 383, 461, 404, 758, 839, 887, 715, 67, 618, 276, 204, 918,
            873, 777, 604, 560, 951, 160, 578, 722, 79, 804, 96, 409, 713, 940, 652, 934,
            970, 447, 318, 353, 859, 672, 112, 785, 645, 863, 803, 350, 139, 93, 354, 99,
            820, 908, 609, 772, 154, 274, 580, 184, 79, 626, 630, 742, 653, 282, 762, 623,
            680, 81, 927, 626, 789, 125, 411, 521, 938, 300, 821, 78, 343, 175, 128, 250,
            170, 774, 972, 275, 999, 639, 495, 78, 352, 126, 857, 956, 358, 619, 580, 124,
            737, 594, 701, 612, 669, 112, 134, 694, 363, 992, 809, 743, 168, 974, 944, 375,
            748, 52, 600, 747, 642, 182, 862, 81, 344, 805, 988, 739, 511, 655, 814, 334,
            249, 515, 897, 955, 664, 981, 649, 113, 974, 459, 893, 228, 433, 837, 553, 268,
            926, 240, 102, 654, 459, 51, 686, 754, 806, 760, 493, 403, 415, 394, 687, 700,
            946, 670, 656, 610, 738, 392, 760, 799, 887, 653, 978, 321, 576, 617, 626, 502,
            894, 679, 243, 440, 680, 879, 194, 572, 640, 724, 926, 56, 204, 700, 707, 151,
            457, 449, 797, 195, 791, 558, 945, 679, 297, 59, 87, 824, 713, 663, 412, 693,
            342, 606, 134, 108, 571, 364, 631, 212, 174, 643, 304, 329, 343, 97, 430, 751,
            497, 314, 983, 374, 822, 928, 140, 206, 73, 263, 980, 736, 876, 478, 430, 305,
            170, 514, 364, 692, 829, 82, 855, 953, 676, 246, 369, 970, 294, 750, 807, 827,
            150, 790, 288, 923, 804, 378, 215, 828, 592, 281, 565, 555, 710, 82, 896, 831,
            547, 261, 524, 462, 293, 465, 502, 56, 661, 821, 976, 991, 658, 869, 905, 758,
            745, 193, 768, 550, 608, 933, 378, 286, 215, 979, 792, 961, 61, 688, 793, 644,
            986, 403, 106, 366, 905, 644, 372, 567, 466, 434, 645, 210, 389, 550, 919, 135,
            780, 773, 635, 389, 707, 100, 626, 958, 165, 504, 920, 176, 193, 713, 857, 265,
            203, 50, 668, 108, 645, 990, 626, 197, 510, 357, 358, 850, 858, 364, 936, 638
        };

        private final BZip2BitInputStream bitInputStream;
        private final CRC32 crc = new CRC32();
        private final int blockCRC;
        private final boolean blockRandomised;
        private int huffmanEndOfBlockSymbol;
        private final byte[] huffmanSymbolMap = new byte[256];
        private final int[] bwtByteCounts = new int[256];
        private byte[] bwtBlock;
        private int[] bwtMergedPointers;
        private int bwtCurrentMergedPointer;
        private int bwtBlockLength;
        private int bwtBytesDecoded;
        private int rleLastDecodedByte = -1;
        private int rleAccumulator;
        private int rleRepeat;
        private int randomIndex = 0;
        private int randomCount = RNUMS[0] - 1;

        private BZip2HuffmanStageDecoder readHuffmanTables() throws IOException
        {
            final BZip2BitInputStream bitInputStream = this.bitInputStream;
            final byte[] huffmanSymbolMap = this.huffmanSymbolMap;

            final byte[][] tableCodeLengths = new byte[BZip2Constants.HUFFMAN_MAXIMUM_TABLES][
                                               BZip2Constants.HUFFMAN_MAXIMUM_ALPHABET_SIZE];

            int huffmanUsedRanges = bitInputStream.readBits (16);
            int huffmanSymbolCount = 0;

            for (int i = 0; i < 16; i++)
            {
                if ((huffmanUsedRanges & ((1 << 15) >>> i)) != 0)
                {
                    for (int j = 0, k = i << 4; j < 16; j++, k++)
                    {
                        if (bitInputStream.readBoolean()) {
                            huffmanSymbolMap[huffmanSymbolCount++] = (byte)k;
                        }
                    }
                }
            }
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
            for (int selector = 0; selector < totalSelectors; selector++) {
                selectors[selector] = tableMTF.indexToFront (bitInputStream.readUnary());
            }

            /* Read the Canonical Huffman code lengths for each table */
            for (int table = 0; table < totalTables; table++)
            {
                int currentLength = bitInputStream.readBits (5);
                for (int i = 0; i <= endOfBlockSymbol; i++) {
                    while (bitInputStream.readBoolean()) {
                        currentLength += bitInputStream.readBoolean() ? -1 : 1;
                    }
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

                if (nextSymbol == BZip2Constants.HUFFMAN_SYMBOL_RUNA) {
                repeatCount += repeatIncrement;
                repeatIncrement <<= 1;
                } else if (nextSymbol == BZip2Constants.HUFFMAN_SYMBOL_RUNB) {
                repeatCount += repeatIncrement << 1;
                repeatIncrement <<= 1;
                } else {
                if (repeatCount > 0) {
                    if (bwtBlockLength + repeatCount > streamBlockSize) {
                        throw new BZip2Exception ("BZip2 block exceeds declared block size");
                    }
                    final byte nextByte = huffmanSymbolMap[mtfValue];
                    bwtByteCounts[nextByte & 0xff] += repeatCount;
                    while (--repeatCount >= 0) {
                        bwtBlock[bwtBlockLength++] = nextByte;
                    }

                    repeatCount = 0;
                    repeatIncrement = 1;
                }

                if (nextSymbol == huffmanEndOfBlockSymbol)
                    break;

                if (bwtBlockLength >= streamBlockSize) {
                    throw new BZip2Exception ("BZip2 block exceeds declared block size");
                }

                mtfValue = symbolMTF.indexToFront (nextSymbol - 1) & 0xff;

                final byte nextByte = huffmanSymbolMap[mtfValue];
                bwtByteCounts[nextByte & 0xff]++;
                bwtBlock[bwtBlockLength++] = nextByte;
                }
            }

            this.bwtBlockLength = bwtBlockLength;
        }

        private void initialiseInverseBWT (final int bwtStartPointer) throws IOException {

        final byte[] bwtBlock  = this.bwtBlock;
        final int[] bwtMergedPointers = new int[this.bwtBlockLength];
        final int[] characterBase = new int[256];

        if ((bwtStartPointer < 0) || (bwtStartPointer >= this.bwtBlockLength)) {
            throw new BZip2Exception ("BZip2 start pointer invalid");
        }

        // Cumulatise character counts
        System.arraycopy (this.bwtByteCounts, 0, characterBase, 1, 255);
        for (int i = 2; i <= 255; i++) {
            characterBase[i] += characterBase[i - 1];
        }

        // Merged-Array Inverse Burrows-Wheeler Transform
        // Combining the output characters and forward pointers into a single array here, where we
        // have already read both of the corresponding values, cuts down on memory accesses in the
        // final walk through the array
        for (int i = 0; i < this.bwtBlockLength; i++) {
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

        if (this.blockRandomised) {
            if (--this.randomCount == 0) {
                nextDecodedByte ^= 1;
                this.randomIndex = (this.randomIndex + 1) % 512;
                this.randomCount = RNUMS[this.randomIndex];
            }
        }

        this.bwtBytesDecoded++;

        return nextDecodedByte;
        }

        public int read()
        {
        while (this.rleRepeat < 1) {

            if (this.bwtBytesDecoded == this.bwtBlockLength) {
                return -1;
            }

            int nextByte = decodeNextBWTByte();

            if (nextByte != this.rleLastDecodedByte) {
                // New byte, restart accumulation
                this.rleLastDecodedByte = nextByte;
                this.rleRepeat = 1;
                this.rleAccumulator = 1;
                this.crc.updateCRC (nextByte);
            } else {
                if (++this.rleAccumulator == 4) {
                    // Accumulation complete, start repetition
                    int rleRepeat = decodeNextBWTByte() + 1;
                    this.rleRepeat = rleRepeat;
                    this.rleAccumulator = 0;
                    this.crc.updateCRC (nextByte, rleRepeat);
                } else {
                    this.rleRepeat = 1;
                    this.crc.updateCRC (nextByte);
                }
            }

        }

        this.rleRepeat--;

        return this.rleLastDecodedByte;
        }

        public int read (final byte[] destination, int offset, final int length)
        {

        int i;
        for (i = 0; i < length; i++, offset++) {
            int decoded = read();
            if (decoded == -1) {
                return (i == 0) ? -1 : i;
            }
            destination[offset] = (byte)decoded;
        }
        return i;
        }

        public int checkCRC() throws IOException {

        if (this.blockCRC != this.crc.getCRC()) {
            throw new BZip2Exception ("BZip2 block CRC error");
        }

        return this.crc.getCRC();
        }

        public BZip2BlockDecompressor(final BZip2BitInputStream bitInputStream,
                            final int blockSize) throws IOException
        {

        this.bitInputStream = bitInputStream;
        this.bwtBlock = new byte[blockSize];

        final int bwtStartPointer;

        // Read block header
        this.blockCRC = this.bitInputStream.readInteger();
        this.blockRandomised = this.bitInputStream.readBoolean();
        bwtStartPointer = this.bitInputStream.readBits (24);

        // Read block data and decode through to the Inverse Burrows Wheeler Transform stage
        BZip2HuffmanStageDecoder huffmanDecoder = readHuffmanTables();
        decodeHuffmanData (huffmanDecoder);
        initialiseInverseBWT (bwtStartPointer);
        }
    }

    class BZip2InputStream extends InputStream
    {
        private InputStream inputStream;
        private BZip2BitInputStream bitInputStream;
        private final boolean headerless;
        private boolean streamComplete = false;
        private int streamBlockSize;
        private int streamCRC = 0;
        private BZip2BlockDecompressor blockDecompressor = null;
    
        @Override public int read() throws IOException
        {
            int nextByte = -1;
            if (this.blockDecompressor == null)
                initialiseStream();
            else
                nextByte = this.blockDecompressor.read();
    
            if (nextByte == -1)
                if (initialiseNextBlock())
                    nextByte = this.blockDecompressor.read();
    
            return nextByte;
        }
    
        @Override
        public int read (final byte[] destination, final int offset, final int length)
            throws IOException
        {
            int bytesRead = -1;
            if (this.blockDecompressor == null)
                initialiseStream();
            else
                bytesRead = this.blockDecompressor.read (destination, offset, length);
    
            if (bytesRead == -1)
                if (initialiseNextBlock())
                    bytesRead = this.blockDecompressor.read (destination, offset, length);
    
            return bytesRead;
        }

        @Override public void close() throws IOException
        {
            if (this.bitInputStream != null)
            {
                this.streamComplete = true;
                this.blockDecompressor = null;
                this.bitInputStream = null;

                try {
                    this.inputStream.close();
                } finally {
                    this.inputStream = null;
                }
            }

        }

        private void initialiseStream() throws IOException
        {
    
            if (this.bitInputStream == null)
                throw new BZip2Exception ("Stream closed");
    
            if (this.streamComplete)
                return;
    
            try {
                int marker1 = this.headerless ? 0 : this.bitInputStream.readBits (16);
                int marker2 = this.bitInputStream.readBits (8);
                int blockSize = (this.bitInputStream.readBits (8) - '0');
    
                if (
                           (!this.headerless && (marker1 != BZip2Constants.STREAM_START_MARKER_1))
                        || (marker2 != BZip2Constants.STREAM_START_MARKER_2)
                        || (blockSize < 1) || (blockSize > 9))
                {
                    throw new BZip2Exception ("Invalid BZip2 header");
                }
    
                this.streamBlockSize = blockSize * 100000;
            } catch (IOException e) {
                // If the stream header was not valid, stop trying to read more data
                this.streamComplete = true;
                throw e;
            }
        }

        private boolean initialiseNextBlock() throws IOException
        {
            if (this.streamComplete)
            return false;

            if (this.blockDecompressor != null) {
            int blockCRC = this.blockDecompressor.checkCRC();
            this.streamCRC = ((this.streamCRC << 1) | (this.streamCRC >>> 31)) ^ blockCRC;
            }

            /* Read block-header or end-of-stream marker */
            final int marker1 = this.bitInputStream.readBits (24);
            final int marker2 = this.bitInputStream.readBits (24);

            if (marker1 == BZip2Constants.BLOCK_HEADER_MARKER_1 &&
                marker2 == BZip2Constants.BLOCK_HEADER_MARKER_2)
            {
                // Initialise a new block
                try {
                    this.blockDecompressor = new BZip2BlockDecompressor(
                                                    this.bitInputStream, this.streamBlockSize);
                } catch (IOException e) {
                    // If the block could not be decoded, stop trying to read more data
                    this.streamComplete = true;
                    throw e;
                }
                return true;
            }
            else if (marker1 == BZip2Constants.STREAM_END_MARKER_1 &&
                     marker2 == BZip2Constants.STREAM_END_MARKER_2)
            {
                // Read and verify the end-of-stream CRC
                this.streamComplete = true;
                final int storedCombinedCRC = this.bitInputStream.readInteger();
                if (storedCombinedCRC != this.streamCRC) {
                    throw new BZip2Exception ("BZip2 stream CRC error");
                }
                return false;
            }

            this.streamComplete = true;
            throw new BZip2Exception ("BZip2 stream format error");
        }

        public BZip2InputStream (final InputStream inputStream, final boolean headerless)
        {
            if (inputStream == null)
                throw new IllegalArgumentException ("Null input stream");

            this.inputStream = inputStream;
            this.bitInputStream = new BZip2BitInputStream (inputStream);
            this.headerless = headerless;
        }
    }

    void submain(String[] args) throws IOException
    {
        if (args.length == 0)
        {
            System.err.println(
                "Demonstration BZip2 decompressor\n\nUsage:\n  java demo.Decompress <filename>\n");

            System.exit (1);
        }

        File inputFile = new File (args[0]);
        if (!inputFile.exists() || !inputFile.canRead() || !args[0].endsWith(".bz2")) {
            System.err.println ("Cannot read file " + inputFile.getPath());
            System.exit (1);
        }

        File outputFile = new File (args[0].substring (0, args[0].length() - 4));
        if (outputFile.exists()) {
            System.err.println ("File " + outputFile.getPath() + " already exists");
            System.exit (1);
        }

        InputStream fileInputStream = new BufferedInputStream (new FileInputStream (inputFile));
        BZip2InputStream inputStream = new BZip2InputStream (fileInputStream, false);
        OutputStream fileOutputStream = new BufferedOutputStream (new FileOutputStream (outputFile), 524288);

        byte[] decoded = new byte [524288];
        int bytesRead;
        while ((bytesRead = inputStream.read (decoded)) != -1) {
            fileOutputStream.write (decoded, 0, bytesRead) ;
        }
        fileOutputStream.close();
    }

    public static void main (String[] args) throws IOException
    {
        new Bzcat().submain(args);


    }

}

