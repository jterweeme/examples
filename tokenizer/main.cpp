//file: tokenizer/main.cpp

#include <string>
#include <iostream>

template <class T> class IYield
{
public:
    virtual bool hasNext() = 0;
    virtual T next() = 0;

    virtual void reset()
    {
        throw "not implemented";
    }
};

class Tokenizer : public IYield<std::string>
{
private:
    const char *_s;
    const char *_ptr;
    char _delim;
    std::string _next;
public:
    Tokenizer(const char *s, char delim);
    bool hasNext() override;
    void reset() override;
    std::string next() override;
};

Tokenizer::Tokenizer(const char *s, char delim) : _s(s), _ptr(s), _delim(delim)
{

}

bool Tokenizer::hasNext()
{
    //gebruiker roept meermaals hasNext() aan
    if (_next.empty() == false)
        return true;

    //sla eerst delimiters over
    while (*_ptr == _delim)
        _ptr++;

    //sla characters op tot aan delimiter
    while (*_ptr != 0 && *_ptr != _delim)
        _next.push_back(*_ptr++);

    return _next.empty() ? false : true;
}

std::string Tokenizer::next()
{
    std::string ret = _next;
    _next.clear();
    return ret;
}

void Tokenizer::reset()
{
    _ptr = _s;
}

void tokenPrinter(std::ostream &os, const char *s, char delim)
{
    Tokenizer t(s, delim);

    while (t.hasNext())
        os << t.next() << "\r\n";

    os.flush();
}

int main()
{
    tokenPrinter(std::cout, "  The quick  brown fox  ", ' ');

    return 0;
}

