#include "toolbox.h"
#include <vector>
#include <fstream>

class Options
{
private:
    bool _fStdin;
    std::vector<std::string> _files;
public:
    typedef std::vector<std::string>::iterator fi;
    Options();
    bool fStdin() const;
    void parse(int argc, char **argv);
    void parse(const wchar_t *lpCmdLine);
    fi begin();
    fi end();
};

Options::Options() : _fStdin(false)
{

}

bool Options::fStdin() const
{
    return _fStdin;
}

void Options::parse(const wchar_t *lpCmdLine)
{
    //tijdelijke quick & dirty oplossing
    if (wcslen(lpCmdLine) < 2)
    {
        _fStdin = true;
        return;
    }

    _files.push_back(Toolbox::wstrtostr(lpCmdLine));
}

void Options::parse(int argc, char **argv)
{
    if (argc == 1)
    {
        _fStdin = true;
        return;
    }

    for (int i = 1; i < argc; ++i)
        _files.push_back(argv[i]);
}

Options::fi Options::begin()
{
    return _files.begin();
}

Options::fi Options::end()
{
    return _files.end();
}

static void hexDump(std::ostream &os, std::istream &is)
{
    char buf[16];
    Toolbox t;
    (void)t;

    while (is.good())
    {
        is.read(buf, 16);
        std::streamsize gcnt = is.gcount();

        for (std::streamsize i = 0; i < gcnt; ++i)
            os << t.hex8(buf[i]) << " ";

        //vul ruimte op in geval minder dan 16 bytes over
        for (std::streamsize i = gcnt; i < 16; ++i)
            os << "   ";

        os << " >";

        for (std::streamsize i = 0; i < gcnt; ++i)
            os.put(isprint(buf[i]) ? buf[i] : '.');

        os << std::endl;
    }
}

#ifdef WINCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    Options options;
    options.parse(lpCmdLine);

    if (options.fStdin())
        hexDump(std::cout, std::cin);

    for (Options::fi it = options.begin(); it != options.end(); ++it)
    {
        std::ifstream ifs;
        ifs.open(it->c_str(), std::ifstream::in | std::ifstream::binary);
        hexDump(std::cout, ifs);
        ifs.close();
    }

    return 0;
}
#else
int main(int argc, char **argv)
{
    Options options;
    options.parse(argc, argv);

    if (options.fStdin())
        hexDump(std::cout, std::cin);

    for (Options::fi it = options.begin(); it != options.end(); ++it)
    {
        std::ifstream ifs;
        ifs.open(*it, std::ifstream::in | std::ifstream::binary);
        hexDump(std::cout, ifs);
        ifs.close();
    }

    return 0;
}
#endif

