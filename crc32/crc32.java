import java.io.IOException;

public class crc32
{
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
/*
            for (int i = 0; i < 256; i++)
                System.err.println(String.format("0x%08X", _table[i]));
*/
        }

        CRC32() { _makeTable(); }
        void update(int c) { _crc = _table[(_crc ^ c) & 0xff] ^ (_crc >>> 8); }
        int crc() { return ~_crc; }
    }

    void submain(String[] args) throws IOException
    {
        CRC32 crc = new CRC32();
        
        for (int c; (c = System.in.read()) != -1;)
            crc.update(c);

        System.out.println(String.format("0x%08X", crc.crc()));
    }

    public static void main(String[] args) throws IOException
    {
        new crc32().submain(args);
    }
}


