//file: bitinput.h

#ifndef BITINPUT_H
#define BITINPUT_H

#include <iostream>

class BitInputStream
{
private:
    std::istream *_is;
    long _bitBuffer;
    int _bitBufferLen;
public:
    void alignToByte();
    bool peek();
    uint32_t readUint(int n);
    int readByte();
    int readSignedInt(int n);
    long readRiceSignedInt(int param);
};

#endif

