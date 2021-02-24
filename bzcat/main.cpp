#include "bitstream.h"
#include "table.h"
#include "block.h"
#include "stream.h"
#include <fstream>

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

class Options
{
private:
    bool _stdin;
    std::string _fn;
public:
    Options();
    bool stdInput() const;
    void parse(int argc, char **argv);
};

Options::Options() : _stdin(true)
{

}

bool Options::stdInput() const
{
    return _stdin;
}

void Options::parse(int argc, char **argv)
{
    if (argc > 1)
    {
        _stdin = false;
        _fn = std::string(argv[1]);
    }
}

int main(int argc, char **argv)
{
    Options o;
    o.parse(argc, argv);
    std::ifstream ifs;
    std::istream *is = nullptr;

    if (o.stdInput())
    {
        is = &std::cin;
#ifdef WIN32
        _setmode(_fileno(stdin), _O_BINARY);
#endif
    }
    else
    {
        ifs.open(argv[1], std::ios::binary);
        is = &ifs;
    }

    BitInputStream bi(is);
    DecStream ds(&bi);
    ds.extractTo(std::cout);
    ifs.close();
    return 0;
}

