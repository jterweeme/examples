import java.io.IOException;

public class GZcat
{
    //dependencies: none
    class BitInput
    {
        java.io.InputStream _is;
        int _nextBits, _bitPos = 8;
        int _getBitPos() { return _bitPos % 8; }
        BitInput(java.io.InputStream is) { _is = is; }
        boolean readBool() throws IOException { return readBit() == 1; }
        void ignoreBits(int n) throws IOException { while (n-- > 0) readBool(); }
        void ignoreBuf() throws IOException { ignoreBits(_getBitPos()); }
        void ignoreBytes(int n) throws IOException { while (n-- > 0) readByte(); }
        int readByte() throws IOException { return _is.read(); }
        int read(byte[] b) throws IOException { return _is.read(b); }

        int readBit() throws IOException
        {
            if (_bitPos == 8) { _nextBits = _is.read(); _bitPos = 0; }
            return _nextBits >>> _bitPos++ & 1;
        }

        int readInt(int numBits) throws IOException
        {
            int result = 0;

            for (int i = 0; i < numBits; i++)
                result |= readBit() << i;

            return result;
        }
    }

    //dependencies: none
    class Node
    {
        Node left, right;
        final int symbol, type;
        Node(Node l, Node r) { symbol = 0; left = l; right = r; type = 1; }
        Node(int s) { symbol = s; type = 2; }
    }

    //dependencies: none
    class Nau
    {
        int[] _a;

        Nau(int[] a)
        {
            _a = a;
        }

        Nau(int n)
        {
            _a = new int[n];
        }

        Nau(int n, int val)
        {
            _a = new int[n];

            for (int i = 0; i < n; i++)
                _a[i] = val;
        }

        void set(int i, int val)
        {
            _a[i] = val;
        }

        int get(int i)
        {
            return _a[i];
        }

        Nau copyOf(int n)
        {
            return new Nau(java.util.Arrays.copyOf(_a, n));
        }

        Nau copyOfRange(int a, int b)
        {
            return new Nau(java.util.Arrays.copyOfRange(_a, a, b));
        }

        int length()
        {
            return _a.length;
        }

        int max()
        {
            int r = _a[0];
            
            for (int x : _a)
                r = Math.max(x, r);

            return r;
        }
    }

    //dependencies: none
    class CircularDict
    {
        private byte[] _data;
        private int _index;
        private final int _mask;

        CircularDict(int size)
        {
            _data = new byte[size];
            _index = 0;
            _mask = (size & (size - 1)) == 0 ? size - 1 : 0;
        }
    
        void append(int b)
        {
            _data[_index] = (byte)b;
            _index = _mask != 0 ? _index + 1 & _mask : (_index + 1) % _data.length;
        }

        void copy(int d, int l, java.io.OutputStream os) throws IOException
        {
            for (int ri = _index - d + _data.length & _mask; l > 0 && _mask != 0; l--)
            {
                os.write(_data[ri]);
                _data[_index] = _data[ri];
                ri = (ri + 1) & _mask;
                _index = (_index + 1) & _mask;
            }

            for (int j = (_index - d + _data.length) % _data.length; l > 0 && _mask == 0; l--)
            {
                os.write(_data[j]);
                _data[_index] = _data[j];
                j = (j + 1) % _data.length;
                _index = (_index + 1) % _data.length;
            }
        }
    }

    //dependencies: Node
    class Pair2
    {
        Node a, b;

        Pair2(Node a, Node b)
        {
            this.a = a;
            this.b = b;
        }
    }

    //dependencies: BitInput, CircularDict, Nau, Node, Pair2
    class Decompressor
    {
        private BitInput _bi;
        private final CircularDict _dict;
        private final Node _lit, _dist;

        private Node _toct(Nau n)
        {
            java.util.List<Node> nodes = new java.util.ArrayList<Node>();
        
            for (int i = n.max(); i >= 1; i--)
            {
                java.util.List<Node> newNodes = new java.util.ArrayList<Node>();
            
                for (int j = 0; j < n.length(); j++)
                    if (n.get(j) == i)
                        newNodes.add(new Node(j));

                for (int j = 0; j < nodes.size(); j += 2)
                    newNodes.add(new Node(nodes.get(j), nodes.get(j + 1)));

                nodes = newNodes;
            }
            return new Node(nodes.get(0), nodes.get(1));
        }

        void extractTo(java.io.OutputStream os) throws IOException
        {
            for (boolean isFinal = false; !isFinal;)
            {
                isFinal = _bi.readBool();
                int type = _bi.readInt(2);

                switch (type)
                {
                case 0:
                    _decRaw(os);
                    break;
                case 1:
                    _decHuff(_lit, _dist, os);
                    break;
                case 2:
                    Pair2 temp = _makePair();
                    _decHuff(temp.a, temp.b, os);
                    break;
                default:
                    throw new AssertionError();
                }
            }
        }

        Decompressor(BitInput in)
        {
            _dict = new CircularDict(32 * 1024);
            int[] llcodelens = new int[288];
            int i = 0;
            for (; i < 144; i++) llcodelens[i] = 8;
            for (; i < 256; i++) llcodelens[i] = 9;
            for (; i < 280; i++) llcodelens[i] = 7;
            for (; i < 288; i++) llcodelens[i] = 8;
            Nau llcodeLens2 = new Nau(llcodelens);
            _lit = _toct(llcodeLens2);
            Nau distCodeLens2 = new Nau(32, 5);
            _dist = _toct(distCodeLens2);
            _bi = in;
        }

        private Pair2 _makePair() throws IOException
        {
            int nlit = _bi.readInt(5) + 257;
            int ndist = _bi.readInt(5) + 1;
            int ncode = _bi.readInt(4) + 4;
            Nau a = new Nau(19);
            a.set(16, _bi.readInt(3));
            a.set(17, _bi.readInt(3));
            a.set(18, _bi.readInt(3));
            a.set(0, _bi.readInt(3));

            for (int i = 0; i < ncode - 4; i++)
                a.set(i % 2 == 0 ? 8 + i / 2 : 7 - i / 2, _bi.readInt(3));

            Node b = _toct(a);
            Nau c = new Nau(nlit + ndist);

            for (int i = 0, runVal = -1, runLen = 0, sym; i < c.length(); i++)
            {
                if (runLen > 0)
                {
                    c.set(i, runVal);
                    runLen--;
                }
                else
                {
                    switch (sym = _decSym(b))
                    {
                    case 16:
                        runLen = _bi.readInt(2) + 3;
                        i--;
                        break;
                    case 17:
                        runVal = 0;
                        runLen = _bi.readInt(3) + 3;
                        i--;
                        break;
                    case 18:
                        runVal = 0;
                        runLen = _bi.readInt(7) + 11;
                        i--;
                        break;
                    default:
                        c.set(i, sym);
                        runVal = sym;
                    }
                }
            }

            Nau litLenCodeLen = c.copyOf(nlit);
            Nau distCodeLen = c.copyOfRange(nlit, c.length());
            return new Pair2(_toct(litLenCodeLen), _toct(distCodeLen));
        }

        private void _decRaw(java.io.OutputStream os) throws IOException
        {
            _bi.ignoreBuf();
            int len = _bi.readInt(16);
            _bi.ignoreBits(16);

            for (int i = 0; i < len; i++)
            {
                int temp = _bi.readByte();
                os.write(temp);
                _dict.append(temp);
            }
        }

        private void _decHuff(Node lit, Node dist, java.io.OutputStream os) throws IOException
        {
            for (int sym; (sym = _decSym(lit)) != 256;)
            {
                if (sym < 256)
                {
                    os.write(sym);
                    _dict.append(sym);
                }
                else
                {
                    int len = _decRll(sym), distSym = _decSym(dist);
                    _dict.copy(_decDist(distSym), len, os);
                }
            }
        }

        private int _decSym(Node n) throws IOException
        {
            Node next = _bi.readBool() ? n.right : n.left;

            for (n = next; next.type == 1; n = next)
                next = _bi.readBool() ? n.right : n.left;

            return next.symbol;
        }

        private int _decRll(int sym) throws IOException
        {
            int i = (sym - 261) / 4;

            if (sym <= 264)
                return sym - 254;

            return sym <= 284 ? ((sym - 265) % 4 + 4 << i) + 3 + _bi.readInt(i) : 258;
        }

        private int _decDist(int sym) throws IOException
        {
            int i = sym / 2 - 1;
            return sym <= 3 ? sym + 1 : (sym % 2 + 2 << i) + 1 + _bi.readInt(i);
        }
    }

    //dependencies: BitInput, CircularDict, Decompressor, Nau, Node, Pair2
    class GzipStream
    {
        private BitInput _bi;
        GzipStream(BitInput bi) { _bi = bi; }

        void extractTo(java.io.OutputStream os) throws IOException
        {
            byte[] x = new byte[10];
            _bi.read(x);
            byte flags = x[3];

            if ((flags & 4) != 0)
            {
                byte[] b = new byte[2];
                _bi.read(b);
                int len = b[0] & 0xFF | (b[1] & 0xFF) << 8;
                _bi.read(new byte[len]);
            }

            if ((flags & 8) != 0)
                System.err.println("File name: " + _readString());

            if ((flags & 2) != 0)
                _bi.ignoreBytes(2);

            if ((flags & 0x10) != 0)
                System.err.println("Comment: " + _readString());

            Decompressor d = new Decompressor(_bi);
            d.extractTo(os);
            os.flush();
        }

        private String _readString() throws IOException
        {
            StringBuilder sb = new StringBuilder();

            for (int c = _bi.readByte(); c != 0; c = _bi.readByte())
                sb.append((char)c);

            return sb.toString();
        }
    }

    public void run(String[] args) throws Exception
    {
        java.io.FileInputStream ifs = new java.io.FileInputStream(args[0]);
        java.io.BufferedInputStream bis = new java.io.BufferedInputStream(ifs, 16 * 1024);
        BitInput bi = new BitInput(bis);
        GzipStream gz = new GzipStream(bi);
        gz.extractTo(System.out);
        bis.close();
    }

    public static void main(String[] args) throws Exception
    {
        new GZcat().run(args);
    }
}


