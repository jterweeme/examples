#ifndef BITSTREAM_H
#define BITSTREAM_H
#include <iostream>

class BitInputBase
{
public:
    virtual uint32_t readBit() = 0;
    virtual uint32_t readBits(uint32_t n) = 0;
    bool readBool();
    void ignore(int n);
    uint8_t readByte();
    uint32_t readUnary();
    void read(uint8_t *s, int n);
    void ignoreBytes(int n);
    uint32_t readUInt32();
    uint32_t readInt();
};

class BitInput : public BitInputBase
{
protected:
    uint32_t _bitBuffer = 0, _bitCount = 0;
    virtual int _getc() = 0;
public:
    uint32_t readBits(uint32_t count);
    uint32_t readBit();
};

class BitInputStream : public BitInput
{
private:
    std::istream *_is;
public:
    BitInputStream(std::istream *is);
protected:
    int _getc() override;
};

#endif


