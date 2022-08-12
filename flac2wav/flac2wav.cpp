//file: flac2wav.cpp

#include <iostream>
#include "toolbox.h"

class FlacFrame
{
private:
    Matrix<int64_t> *_samples;
    int _blockSize;
    int _numChannels;
    int _sampleDepth;
    void _decodeSubframe(BitInputStream &in, int sampleDepth, int ch);
    void _decodeResiduals(BitInputStream &in, int warmup, int ch);
    void _restoreLinearPrediction(int ch, const int *coefs, int shift, int length);
public:
    FlacFrame(int numChannels, int sampleDepth);
    void decode(BitInputStream &in);
    void write(std::ostream &os);
};

void FlacFrame::_decodeResiduals(BitInputStream &in, int warmup, int ch)
{
    int method = in.readUint(2);

    if (method >= 2)
        throw "Reserved residual coding method";

    int paramBits = method == 0 ? 4 : 5;
    int escapeParam = method == 0 ? 0xF : 0x1F;
    int partitionOrder = in.readUint(4);
    int numPartitions = 1 << partitionOrder;

    if (_blockSize % numPartitions != 0)
        throw "Block size not divisible by number of Rice partitions";

    int partitionSize = _blockSize / numPartitions;

    for (int i = 0; i < numPartitions; i++)
    {
        int start = i * partitionSize + (i == 0 ? warmup : 0);
        int end = (i + 1) * partitionSize;
        int param = in.readUint(paramBits);

        if (param < escapeParam)
        {
            for (int j = start; j < end; j++)
                _samples->set(ch, j, in.readRiceSignedInt(param));
        }
        else
        {
            int numBits = in.readUint(5);

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

void FlacFrame::_restoreLinearPrediction(int ch, const int *coefs, int shift, int length)
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
    int type = in.readUint(6);
    int shift = in.readUint(1);

    if (shift == 1)
    {
        while (in.readUint(1) == 0)
            shift++;
    }

    sampleDepth -= shift;
    std::cerr << "Type: " << type << "\r\n";
    std::cerr.flush();

    if (type == 0)
    {
        // Constant coding
        for (unsigned i = 0; i < _blockSize; ++i)
            _samples->set(ch, i, in.readSignedInt(sampleDepth));
    }
    else if (type == 1)
    {
        // Verbatim coding
        for (unsigned i = 0; i < _blockSize; ++i)
            _samples->set(ch, i, in.readSignedInt(sampleDepth));
    }
    else if (8 <= type && type <= 12)
    {
        for (int i = 0; i < _blockSize; ++i)
            _samples->set(ch, i, in.readSignedInt(sampleDepth));

        _decodeResiduals(in, type - 8, ch);
        _restoreLinearPrediction(ch, FIXED_PREDICTION_COEFFICIENTS[type - 8], 0, type - 8);
    }
    else if (32 <= type && type <= 63)
    {
        for (int i = 0; i < type - 31; ++i)
            _samples->set(ch, i, in.readSignedInt(sampleDepth));

        int precision = in.readUint(4) + 1;
        int shift2 = in.readSignedInt(5);
        int coefs[type - 31];

        for (int i = 0; i < type - 31; ++i)
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
    int temp = in.readByte();
    int sync = temp << 6 | in.readUint(6);

    std::cerr << "Sync: " << sync << "\r\n";
    std::cerr.flush();

    if (sync != 0x3ffe)
        throw "Sync code expected";

    in.readUint(1);
    in.readUint(1);
    int blockSizeCode = in.readUint(4);
    int sampleRateCode = in.readUint(4);
    int chanAsgn = in.readUint(4);
    in.readUint(3);
    in.readUint(1);
    temp = Toolbox::numberOfLeadingZeros(~(in.readUint(8) << 24)) - 1;

    for (int i = 0; i < temp; ++i)
        in.readUint(8);

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
                val += 128;

            os << _sampleDepth / 8;
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
    if (in.readUint(32) != 0x664c6143)
        throw "Invalid magic string";

    int sampleRate = -1;
    int numChannels = -1;
    int sampleDepth = -1;
    int64_t numSamples = -1;

    for (bool last = false; !last;)
    {
        last = in.readUint(1) != 0;
        int type = in.readUint(7);
        int length = in.readUint(24);

        if (type == 0)
        {
            in.readUint(16);
            in.readUint(16);
            in.readUint(24);
            in.readUint(24);
            sampleRate = in.readUint(20);
            numChannels = in.readUint(3) + 1;
            sampleDepth = in.readUint(5) + 1;
            numSamples = (int64_t)in.readUint(18) << 18 | in.readUint(18);

            for (int i = 0; i < 16; i++)
                in.readUint(8);
        }
        else
        {
            for (int i = 0; i < length; i++)
                in.readUint(8);
        }
    }

    if (sampleRate == -1)
        throw "Stream info metadata block absent";

    if (sampleDepth % 8 != 0)
        throw "Sample depth not supported";

    int64_t sampleDataLen = numSamples * numChannels * (sampleDepth / 8);
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
    std::cerr << sampleDataLen << "\r\n";
    std::cerr.flush();

    while (in.peek())
    {
        FlacFrame frame(numChannels, sampleDepth);
        frame.decode(in);
        frame.write(os);
    }
}

int main()
{
    try
    {
        BitInputStream bin(&std::cin);
        decodeFile(bin, std::cout);
    }
    catch (const char *e)
    {
        std::cerr << e << "\r\n";
        std::cerr.flush();
    }
    catch (...)
    {
        std::cerr << "Unknown exception\r\n";
        std::cerr.flush();
    }
    return 0;
}

