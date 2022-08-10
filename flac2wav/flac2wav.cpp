//file: flac2wav.cpp

#include <iostream>
#include <vector>
#include "bitinput.h"
#include "toolbox.h"

static void decodeResiduals(BitInputStream &in, int warmup, int *result, unsigned resultSize)
{
    int method = in.readUint(2);

    if (method >= 2)
        throw std::exception();

    int paramBits = method == 0 ? 4 : 5;
    int escapeParam = method == 0 ? 0xF : 0x1F;

    int partitionOrder = in.readUint(4);
    int numPartitions = 1 << partitionOrder;

    if (resultSize % numPartitions != 0)
        throw std::exception();

    int partitionSize = resultSize / numPartitions;

    for (int i = 0; i < numPartitions; i++)
    {
        int start = i * partitionSize + (i == 0 ? warmup : 0);
        int end = (i + 1) * partitionSize;

        int param = in.readUint(paramBits);

        if (param < escapeParam)
        {
            for (int j = start; j < end; j++)
                result[j] = in.readRiceSignedInt(param);
        }
        else
        {
            int numBits = in.readUint(5);

            for (int j = start; j < end; j++)
                result[j] = in.readSignedInt(numBits);
        }
    }
}

static void restoreLinearPrediction(int *result, int *coefs, int shift)
{
    //for (int i = )
}

static void decodeFixedPredictionSubframe(BitInputStream &in, int predOrder,
    int sampleDepth, int *result, unsigned resultSize)
{
    for (int i = 0; i < predOrder; i++)
        result[i] = in.readSignedInt(sampleDepth);

    decodeResiduals(in, predOrder, result, resultSize);
    restoreLinearPrediction(result, FIXED_PREDICTION_COEFFICIENTS[predOrder], 0);
}

static void decodeLinearPredictiveCodingSubframe(BitInputStream in,
    int lpcOrder, int sampleDepth, int *result)
{
    for (int i = 0; i < lpcOrder; i++)
        result[i] = in.readSignedInt(sampleDepth);

    int precision = in.readUint(4) + 1;
    int shift = in.readSignedInt(5);
    int *coefs = new int[lpcOrder];

    for (int i = 0; i < lpcOrder; i++)
        coefs[i] = in.readSignedInt(precision);

    decodeResiduals(in, lpcOrder, result);
    restoreLinearPrediction(result, coefs, shift);
}

static void decodeSubframe(BitInputStream &in, int sampleDepth, int *result, unsigned resultSize)
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

    if (type == 0)
    {
        // Constant coding
        for (unsigned i = 0; i < resultSize; ++i)
            result[i] = in.readSignedInt(sampleDepth);
    }
    else if (type == 1)
    {
        // Verbatim coding
        for (unsigned i = 0; i < resultSize; ++i)
            result[i] = in.readSignedInt(sampleDepth);
    }
    else if (8 <= type && type <= 12)
    {
        decodeFixedPredictionSubframe(in, type - 8, sampleDepth, result, resultSize);
    }
    else if (32 <= type && type <= 63)
    {
        decodeLinearPredictiveCodingSubframe(in, type - 31, sampleDepth, result);
    }
    else
    {
        throw std::exception();
    }

    for (unsigned i = 0; i < resultSize; ++i)
        result[i] <<= shift;
}

static void decodeSubframes(BitInputStream &in, int sampleDepth, int chanAsgn, Matrix &mat)
{
    int blockSize = mat.width();
    Matrix subframes(mat.width(), blockSize);

    if (chanAsgn <= 7)
    {
        for (unsigned ch = 0; ch < mat.width(); ++ch)
            decodeSubframe(in, sampleDepth, subframes.buf(), subframes.width());
    }
}



static bool decodeFrame(BitInputStream &in, int numChannels, int sampleDepth, std::ostream &os)
{
    int temp = in.readByte();

    if (temp == -1)
        return false;

    int sync = temp << 6 | in.readUint(6);

    if (sync != 0x3ffe)
        throw std::exception();

    in.readUint(1);
    in.readUint(1);
    int blockSizeCode = in.readUint(4);
    int sampleRateCode = in.readUint(4);
    int chanAsgn = in.readUint(4);
    in.readUint(3);
    in.readUint(1);

    temp = Toolbox::numberOfLeadingZeros(~(in.readUint(8) << 24)) - 1;

    for (int i = 0; i < temp; i++)
        in.readUint(8);

    int blockSize;

    if (blockSizeCode == 1)
        blockSize = 192;
    else if (2 <= blockSizeCode && blockSizeCode <= 5)
        blockSize = 576 << (blockSizeCode - 2);
    else if (blockSizeCode == 6)
        blockSize = in.readUint(8) + 1;
    else if (blockSizeCode == 7)
        blockSize = in.readUint(16) + 1;
    else if (8 <= blockSizeCode && blockSizeCode <= 15)
        blockSize = 256 << (blockSizeCode - 8);
    else
        throw std::exception();

    if (sampleRateCode == 12)
        in.readUint(8);
    else if (sampleRateCode == 13 || sampleRateCode == 14)
        in.readUint(16);

    in.readUint(8);

    // Decode each channel's subframe, then skip footer
    Matrix samples(numChannels, blockSize);
    decodeSubframes(in, sampleDepth, chanAsgn, samples);
    //in.alignToByte();
    in.readUint(16);

    // Write the decoded samples
    for (int i = 0; i < blockSize; i++)
    {
        for (int j = 0; j < numChannels; j++)
        {
            int val = samples.at(j, i);

            if (sampleDepth == 8)
                val += 128;

            os << sampleDepth / 8;
        }
    }
    return true;
}

static void decodeFile(BitInputStream &in, std::ostream &os)
{
    if (in.readUint(32) != 0x664c6143)
        throw std::exception();

    int sampleRate = -1;
    int numChannels = -1;
    int sampleDepth = -1;
    long numSamples = -1;

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
            numSamples = (long)in.readUint(18) << 18 | in.readUint(18);

            for (int i = 0; i < 16; i++)
                in.readUint(8);
        }
        else
        {
            for (int i = 0; i < length; i++)
                in.readUint(8);
        }
    }

    long sampleDataLen = numSamples * numChannels * (sampleDepth / 8);
    os << "RIFF";
    os << sampleDataLen + 36;
    os << "WAVE";
    os << "fmt";
    os << 16;
    os << 1;
    os << numChannels;
    os << sampleRate;
    os << sampleRate * numChannels * (sampleDepth / 8);
    os << (uint16_t)(numChannels * (sampleDepth / 8));
    os << sampleDepth;
    os << "data";
    os << sampleDataLen;

    while (decodeFrame(in, numChannels, sampleDepth, os))
    {
        continue;
    }
}

int main()
{
    BitInputStream bin;
    decodeFile(bin, std::cout);
}

