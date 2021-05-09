#include <iostream>

class Options
{
private:
    bool _stdin;
public:
    Options();
    void parse(int argc, char **argv);
    bool stdinput() const;
};

Options::Options() : _stdin(true)
{

}

void Options::parse(int argc, char **argv)
{
    (void)argc;
    (void)argv;
}

bool Options::stdinput() const
{
    return _stdin;
}

static void nl(std::ostream &os, std::istream &is)
{
    uint32_t n = 1;

    while (is)
    {
        os << n << "  ";

        int c;

        c = is.get();

        if (c == 0x0a)
        {
            n++;
            continue;
        }

        os.put(c);
    }
}

int main(int argc, char **argv)
{
    Options o;

    o.parse(argc, argv);

    if (o.stdinput())
    {
        nl(std::cout, std::cin);
    }


    return 0;
}

