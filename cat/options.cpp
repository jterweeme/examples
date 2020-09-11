#include "options.h"
#include "toolbox.h"

Options::si Options::_fnBegin()
{
    return _fileNames.begin();
}

Options::si Options::_fnEnd()
{
    return _fileNames.end();
}

Options::Options() : _stdinput(false)
{

}

void Options::parse(int argc, char **argv)
{
    if (argc < 2)
    {
        _stdinput = true;
        return;
    }

    for (int i = 1; i < argc; ++i)
        _fileNames.push_back(argv[i]);
}

bool Options::stdinput() const
{
    return _stdinput;
}

void Options::parse(const wchar_t *lpCmdLine)
{
    Toolbox::vw args = Toolbox::explode(lpCmdLine, L' ');

    if (args.empty())
    {
        _stdinput = true;
        return;
    }

    for (Toolbox::vwi it = args.begin(); it != args.end(); ++it)
    {
        _fileNames.push_back(Toolbox::wstrtostr(*it));
    }
}

