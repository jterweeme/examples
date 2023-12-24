#include <vector>
#include <iostream>
#include <cassert>

using std::ostream;
using std::string;
using std::cout;
using std::cin;

class PrintStack
{
    std::vector<char> _stack;
public:
    void push_back(char c) { _stack.push_back(c); }
    void print(ostream &os) { for (; _stack.size(); _stack.pop_back()) os.put(_stack.back()); }
};

class LZW
{
    const unsigned _maxbits;
    unsigned _oldcode = 0;
    char _finchar;
    std::vector<std::pair<unsigned, char>> _dict;
    ostream &_os; 
    PrintStack _stack;
public:
    LZW(unsigned maxbits, ostream &os) : _maxbits(maxbits), _os(os) { }

    inline void code(const unsigned in)
    {
        assert(in <= _dict.size() + 256);
        auto c = in;

        if (in == 256)
        {
            _dict.clear();
            return;
        }

        if (c == _dict.size() + 256)
        {
            _stack.push_back(_finchar);
            c = _oldcode;
        }

        for (; c >= 256U; c = _dict[c - 256].first)
        {
            _stack.push_back(_dict[c - 256].second);
        }

        _os.put(_finchar = c);

        if (_dict.size() + 256 < 1U << _maxbits)
        {
            _dict.push_back(std::pair<unsigned, char>(_oldcode, _finchar));
        }

        _oldcode = in;
        _stack.print(_os);
    }
};

int main()
{
    string s;
    LZW lzw(16, cout);

    while (std::getline(cin, s))
        lzw.code(std::stoi(s));

    return 0;
}


