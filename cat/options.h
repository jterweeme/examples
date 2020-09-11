#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <vector>

class Options
{
private:
    bool _stdinput;
    std::vector<std::string> _fileNames;
public:
    Options();
    void parse(int argc, char **argv);
    void parse(const wchar_t *lpCmdLine);
    bool stdinput() const;
    typedef std::vector<std::string>::iterator si;
    si _fnBegin();
    si _fnEnd();
};

#endif

