#include <iostream>
#include <fstream>
#include <vector>

class Parser
{
private:
    int _lines;
    int _words;
    int _chars;
public:
    static const uint8_t LINE_FEED = 0x0a;
    static const uint8_t CARRIAGE_RETURN = 0x0d;
    static const uint8_t SPACE = 0x20;
    Parser();
    void reset();
    void parse(std::istream &is);
    int characters() const;
    int words() const;
    int lines() const;
};

Parser::Parser() : _lines(0), _words(0), _chars(0)
{

}

void Parser::reset()
{
    _lines = 0;
    _words = 0;
    _chars = 0;
}

void Parser::parse(std::istream &is)
{
    bool fLine = false;
    bool fWord = false;

    while (is.good())
    {
        int c = is.get();

        //continue had ook break kunnen zijn
        if (c == EOF)
            continue;

        switch (c)
        {
        case LINE_FEED:
        case CARRIAGE_RETURN:
            if (fLine == false)
                ++_lines;

            if (fWord)
                ++_words;

            fLine = !fLine;
            fWord = false;
            break;
        case SPACE:
            if (fWord)
                ++_words;

            fLine = false;
            fWord = false;
            break;
        default:
            //characters
            fLine = false;
            fWord = true;
            break;
        }

        //std::cout.put(char(c));
        ++_chars;
    }
}

int Parser::characters() const
{
    return _chars;
}

int Parser::words() const
{
    return _words;
}

int Parser::lines() const
{
    return _lines;
}

class Options
{
public:
    typedef std::vector<std::string> vs;
    typedef vs::const_iterator vsci;
private:
    bool _stdin;
    vs _files;
public:
    Options();
    void parse(int argc, char **argv);
    bool stdinput() const;
    vsci begFiles() const;
    vsci endFiles() const;
};

Options::vsci Options::begFiles() const
{
    return _files.cbegin();
}

Options::vsci Options::endFiles() const
{
    return _files.cend();
}

Options::Options() : _stdin(false)
{

}

void Options::parse(int argc, char **argv)
{
    if (argc < 2)
        _stdin = true;

    for (int i = 1; i < argc; ++i)
    {
        _files.push_back(argv[i]);
    }
}

bool Options::stdinput() const
{
    return _stdin;
}

int main(int argc, char **argv)
{
    Options o;
    o.parse(argc, argv);
    Parser p;

    if (o.stdinput())
    {
        p.parse(std::cin);
        std::cout << p.lines() << " " << p.words() << " " << p.characters() << "\n";
        std::cout.flush();
        return 0;
    }

    int totalLines = 0;
    int totalWords = 0;
    int totalChars = 0;

    for (Options::vsci it = o.begFiles(); it != o.endFiles(); ++it)
    {
        p.reset();
        std::ifstream ifs;
        ifs.open(*it, std::ifstream::binary);
        p.parse(ifs);
        ifs.close();
        totalLines += p.lines();
        totalWords += p.words();
        totalChars += p.characters();
        std::cout << p.lines() << " " << p.words() << " " << p.characters() << " " << *it << "\n";
        std::cout.flush();
    }

    std::cout << totalLines << " " << totalWords << " " << totalChars << " total\n";
    std::cout.flush();
    return 0;
}

