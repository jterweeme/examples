//file: flac2wav.cpp

#include <iostream>
#include <fstream>

template <class T> class Matrix
{
private:
    T *_buf;
    unsigned _x, _y;
public:
    Matrix(unsigned x, unsigned y) : _x(x), _y(y) {
        _buf = new T[x * y];
    }

    ~Matrix() {
        delete[] _buf;
    }

    T at(unsigned x, unsigned y) const {
        return *(_buf + x * _y + y);
    }

    void set(unsigned x, unsigned y, T value) {
        *(_buf + x * _y + y) = value;
    }
};

class Toolbox
{
public:
    static void writeWLE(std::ostream &os, uint16_t w);
    static void writeDwLE(std::ostream &os, uint32_t dw);
};

class BitInputStream
{
private:
    std::istream *_is;
    long _bitBuffer = 0;
    int _bitBufferLen = 0;
public:
    BitInputStream(std::istream *is);
    void alignToByte();
    bool peek();
    uint64_t readUint(int n);
    int readByte();
    int64_t readSignedInt(int n);
    int64_t readRiceSignedInt(int param);
};

class FlacFrame
{
private:
    Matrix<int64_t> *_samples;
    uint32_t _blockSize;
    int _numChannels;
    int _sampleDepth;
    void _decodeSubframe(BitInputStream &in, int sampleDepth, int ch);
    void _decodeResiduals(BitInputStream &in, int warmup, int ch);
    void _restoreLinearPrediction(int ch, const int *coefs, uint8_t shift, int length);
public:
    FlacFrame(int numChannels, int sampleDepth);
    ~FlacFrame();
    void decode(BitInputStream &in);
    void write(std::ostream &os);
};

FlacFrame::~FlacFrame()
{
    delete _samples;
}

void FlacFrame::_decodeResiduals(BitInputStream &in, int warmup, int ch)
{
    uint8_t method = in.readUint(2);

    if (method >= 2)
        throw "Reserved residual coding method";

    uint8_t paramBits = method == 0 ? 4 : 5;
    uint8_t escapeParam = method == 0 ? 0xF : 0x1F;
    uint8_t partitionOrder = in.readUint(4);
    uint32_t numPartitions = 1 << partitionOrder;

    if (_blockSize % numPartitions != 0)
        throw "Block size not divisible by number of Rice partitions";

    uint32_t partitionSize = _blockSize / numPartitions;

    for (uint32_t i = 0; i < numPartitions; i++)
    {
        int start = i * partitionSize + (i == 0 ? warmup : 0);
        int end = (i + 1) * partitionSize;
        uint8_t param = in.readUint(paramBits);

        if (param < escapeParam)
        {
            for (int j = start; j < end; j++)
                _samples->set(ch, j, in.readRiceSignedInt(param));
        }
        else
        {
            uint8_t numBits = in.readUint(5);

            for (int j = start; j < end; j++)
                _samples->set(ch, j, in.readSignedInt(numBits));
        }
    }
}

static constexpr int FIXED_PREDICTION_COEFFICIENTS[][4] =
{
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {2, -1, 0, 0},
    {3, -3, 1, 0},
    {4, -6, 4, -1}
};

void FlacFrame::_restoreLinearPrediction(int ch, const int *coefs, uint8_t shift, int length)
{
    for (int i = length; i < _blockSize; ++i)
    {
        int64_t sum = 0;

        for (int j = 0; j < length; ++j)
            sum += _samples->at(ch, i - 1 - j) * coefs[j];

        int64_t temp = _samples->at(ch, i);
        temp += sum >> shift;
        _samples->set(ch, i, temp);
    }
}

void FlacFrame::_decodeSubframe(BitInputStream &in, int sampleDepth, int ch)
{
    in.readUint(1);
    uint8_t type = in.readUint(6);
    uint8_t shift = in.readUint(1);

    if (shift == 1)
    {
        while (in.readUint(1) == 0)
            shift++;
    }

    sampleDepth -= shift;

    if (type == 0)
    {
        int64_t ret = in.readSignedInt(sampleDepth);

        // Constant coding
        for (unsigned i = 0; i < _blockSize; ++i)
            _samples->set(ch, i, ret);
    }
    else if (type == 1)
    {
        // Verbatim coding
        for (unsigned i = 0; i < _blockSize; ++i)
            _samples->set(ch, i, in.readSignedInt(sampleDepth));
    }
    else if (8 <= type && type <= 12)
    {
        for (uint8_t i = 0; i < type - 8; ++i)
        {
            int64_t sample = in.readSignedInt(sampleDepth);
            _samples->set(ch, i, sample);
        }

        _decodeResiduals(in, type - 8, ch);
        _restoreLinearPrediction(ch, FIXED_PREDICTION_COEFFICIENTS[type - 8], 0, type - 8);
    }
    else if (32 <= type && type <= 63)
    {
        for (uint8_t i = 0; i < type - 31; ++i)
            _samples->set(ch, i, in.readSignedInt(sampleDepth));

        uint8_t precision = in.readUint(4) + 1;
        uint8_t shift2 = in.readUint(5);
        int coefs[type - 31];

        for (uint8_t i = 0; i < type - 31; ++i)
            coefs[i] = in.readSignedInt(precision);

        _decodeResiduals(in, type - 31, ch);
        _restoreLinearPrediction(ch, coefs, shift2, type - 31);
    }
    else
    {
        throw "Reserved subframe type";
    }

    for (unsigned i = 0; i < _blockSize; ++i)
    {
        int64_t temp = _samples->at(ch, i);
        temp <<= shift;
        _samples->set(ch, i, temp);
    }
}

void FlacFrame::decode(BitInputStream &in)
{
    {
        int temp = in.readByte();
        int sync = temp << 6 | in.readUint(6);

        if (sync != 0x3ffe)
            throw "Sync code expected";

        in.readUint(1);
        in.readUint(1);
    }
    uint8_t blockSizeCode = in.readUint(4);
    uint8_t sampleRateCode = in.readUint(4);
    uint8_t chanAsgn = in.readUint(4);

    {
        in.readUint(3);
        in.readUint(1);
        uint16_t foo = in.readUint(8);

        while (foo >= 0b11000000)
        {
            in.readUint(8);
            foo = (foo << 1) & 0xff;
        }
    }

    if (blockSizeCode == 1)
        _blockSize = 192;
    else if (2 <= blockSizeCode && blockSizeCode <= 5)
        _blockSize = 576 << (blockSizeCode - 2);
    else if (blockSizeCode == 6)
        _blockSize = in.readUint(8) + 1;
    else if (blockSizeCode == 7)
        _blockSize = in.readUint(16) + 1;
    else if (8 <= blockSizeCode && blockSizeCode <= 15)
        _blockSize = 256 << (blockSizeCode - 8);
    else
        throw "Reserved block size";

    if (sampleRateCode == 12)
        in.readUint(8);
    else if (sampleRateCode == 13 || sampleRateCode == 14)
        in.readUint(16);

    in.readUint(8);
    _samples = new Matrix<int64_t>(_numChannels, _blockSize);

    if (0 <= chanAsgn && chanAsgn <= 7)
    {
        for (int ch = 0; ch < _numChannels; ++ch)
            _decodeSubframe(in, _sampleDepth, ch);
    }
    else if (8 <= chanAsgn && chanAsgn <= 10)
    {
        _decodeSubframe(in, _sampleDepth + (chanAsgn == 9 ? 1 : 0), 0);
        _decodeSubframe(in, _sampleDepth + (chanAsgn == 9 ? 0 : 1), 1);

        if (chanAsgn == 8)
        {
            for (int i = 0; i < _blockSize; ++i)
                _samples->set(1, i, _samples->at(0, i) - _samples->at(1, i));
        }
        else if (chanAsgn == 9)
        {
            for (int i = 0; i < _blockSize; ++i)
                _samples->set(0, i, _samples->at(0, i) + _samples->at(1, i));
        }
        else if (chanAsgn == 10)
        {
            for (int i = 0; i < _blockSize; ++i)
            {
                int64_t side = _samples->at(1, i);
                int64_t right = _samples->at(0, i) - (side >> 1);
                _samples->set(1, i, right);
                _samples->set(0, i, right + side);
            }
        }
    }
    else
    {
        throw "Reserved channel assignment";
    }

    in.alignToByte();
    in.readUint(16);
}

void FlacFrame::write(std::ostream &os)
{
     // Write the decoded samples
    for (int i = 0; i < _blockSize; i++)
    {
        for (int j = 0; j < _numChannels; j++)
        {
            int val = int(_samples->at(j, i));

            if (_sampleDepth == 8)
            {
                val += 128;
                os.put(val % 0xff);
            }
            else if (_sampleDepth == 16)
            {
                Toolbox::writeWLE(os, val);
            }
            else
            {
                throw "Unsupported sample depth";
            }
        }
    }   
}

FlacFrame::FlacFrame(int numChannels, int sampleDepth)
{
    _numChannels = numChannels;
    _sampleDepth = sampleDepth;
}

static void decodeFile(BitInputStream &in, std::ostream &os)
{
    uint32_t magic = in.readUint(32);

    if (magic != 0x664c6143)
        throw "Invalid magic string";

    int sampleRate = -1;
    int numChannels = -1;
    uint8_t sampleDepth = 0;
    uint64_t numSamples = 0;

    for (bool last = false; !last;)
    {
        last = in.readUint(1) != 0;
        uint8_t type = in.readUint(7);
        uint32_t length = in.readUint(24);

        if (type == 0)
        {
            in.readUint(16);
            in.readUint(16);
            in.readUint(24);
            in.readUint(24);
            sampleRate = in.readUint(20);
            numChannels = in.readUint(3) + 1;
            sampleDepth = in.readUint(5) + 1;
            numSamples = in.readUint(18) << 18 | in.readUint(18);

            for (int i = 0; i < 16; i++)
                in.readUint(8);
        }
        else
        {
            for (uint32_t i = 0; i < length; ++i)
                in.readUint(8);
        }
    }

    if (sampleRate == -1)
        throw "Stream info metadata block absent";

    if (sampleDepth % 8 != 0)
        throw "Sample depth not supported";

    uint64_t sampleDataLen = numSamples * numChannels * (sampleDepth / 8);
    os << "RIFF";
    Toolbox::writeDwLE(os, sampleDataLen + 36);
    os << "WAVE";
    os << "fmt ";
    Toolbox::writeDwLE(os, 16);
    Toolbox::writeWLE(os, 1);
    Toolbox::writeWLE(os, numChannels);
    Toolbox::writeDwLE(os, sampleRate);
    Toolbox::writeDwLE(os, sampleRate * numChannels * (sampleDepth / 8));
    Toolbox::writeWLE(os, numChannels * (sampleDepth / 8));
    Toolbox::writeWLE(os, sampleDepth);
    os << "data";
    Toolbox::writeDwLE(os, sampleDataLen);

    while (in.peek())
    {
        FlacFrame frame(numChannels, sampleDepth);
        frame.decode(in);
        frame.write(os);
    }
}

void Toolbox::writeWLE(std::ostream &os, uint16_t w)
{
    os.put(w >> 0 & 0xff);
    os.put(w >> 8 & 0xff);
}

void Toolbox::writeDwLE(std::ostream &os, uint32_t dw)
{
    os.put(dw >>  0 & 0xff);
    os.put(dw >>  8 & 0xff);
    os.put(dw >> 16 & 0xff);
    os.put(dw >> 24 & 0xff);
}

BitInputStream::BitInputStream(std::istream *is) : _is(is)
{
}

void BitInputStream::alignToByte()
{
    _bitBufferLen -= _bitBufferLen % 8;
}

int BitInputStream::readByte()
{
    if (_bitBufferLen >= 8)
        return readUint(8);

    return _is->get();
}

bool BitInputStream::peek()
{
    if (_bitBufferLen > 0)
        return true;

    int temp = _is->get();

    if (temp == -1)
        return false;

    _bitBuffer = (_bitBuffer << 8) | temp;
    _bitBufferLen += 8;
    return true;
}

uint64_t BitInputStream::readUint(int n)
{
    while (_bitBufferLen < n)
    {
        int temp = _is->get();

        if (temp == -1)
            throw std::exception();

        _bitBuffer = (_bitBuffer << 8) | temp;
        _bitBufferLen += 8;
    }

    _bitBufferLen -= n;
    int result = int(_bitBuffer >> _bitBufferLen);

    if (n < 32)
        result &= (1 << n) - 1;

    return result;
}

int64_t BitInputStream::readSignedInt(int n)
{
    return int64_t(readUint(n) << 64 - n) >> 64 - n;
}

int64_t BitInputStream::readRiceSignedInt(int param)
{
    int64_t val = 0;

    while (readUint(1) == 0)
        ++val;
    
    val = (val << param) | readUint(param);
    int64_t ret = (val >> 1) ^ -(val & 1);
    return ret;
}


int main(int argc, char **argv)
{

    try
    {
        std::ifstream ifs(argv[1]);
        std::ofstream ofs(argv[2]);

        BitInputStream bin(&ifs);
        decodeFile(bin, ofs);
    }
    catch (const char *e)
    {
        std::cerr << e << "\r\n";
        std::cerr.flush();
    }
    catch (...)
    {
        std::cerr << "Onbekend exception\r\n";
        std::cerr.flush();
    }
    return 0;
}

