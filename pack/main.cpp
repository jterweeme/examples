/*
 *	Huffman encoding program
 *	Adapted April 1979, from program by T.G. Szymanski, March 1978
 *	Usage:	pack [[ - ] filename ... ] filename ...
 *		- option: enable/disable listing of statistics
 */

#include "pack.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <exception>

class Options
{
private:
    std::vector<std::string> _files;
public:
    typedef std::vector<std::string>::const_iterator fit;
    void parse(int argc, char **argv);
    fit fcbegin() const;
    fit fcend() const;
};

void Options::parse(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        _files.push_back(argv[i]);
    }
}

Options::fit Options::fcbegin() const
{
    return _files.cbegin();
}

Options::fit Options::fcend() const
{
    return _files.cend();
}

static void packfile(std::string fn, std::ostream &msgs)
{
    msgs << fn << "\r\n";
    std::ifstream ifs;
    ifs.open(fn, std::ifstream::binary);

    if (ifs.is_open() == false)
        throw std::runtime_error("Cannot open input file");

    fn.append(".z");
    std::ofstream ofs;
    ofs.open(fn, std::ofstream::binary);
    long insize = 0;
    long outsize = 0;
    pack(ifs, ofs, insize, outsize);
    msgs << ": " << insize << " in, " << outsize << " out\r\n";
    ofs.close();
    ifs.close();
}

int main(int argc, char **argv)
{
#if 0
    Options o;
    o.parse(argc, argv);
    int failcount = 0;

    for (Options::fit it = o.fcbegin(); it != o.fcend(); ++it)
    {
        try
        {
            packfile(*it, std::cout);
        }
        catch (...)
        {
            failcount++;
            std::cerr << "Error\r\n";
        }
    }

    return failcount;
#else
    (void)argc;
    (void)argv;

    try
    {
        packfile("d:\\temp\\50sport.iso", std::cout);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\r\n";
    }
    catch (...)
    {
        std::cerr << "Unspecified error\r\n";
    }

    return 0;
#endif
}



