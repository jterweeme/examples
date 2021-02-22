#ifndef STREAM_H
#define STREAM_H

#include "block.h"

class DecStream
{
private:
    BitInputBase *_bi;
    Block _bd;
    uint32_t _streamComplete = false;
    bool _initNextBlock();
public:
    int read();
    int read(char *buf, int n);
    DecStream(BitInputBase *bi);
    void extractTo(std::ostream &os);
};
#endif

