#include <exception>
#include <iostream>

void test()
{
    throw std::exception();
}

int main()
{
    try
    {
        test();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        std::cerr.flush();
    }
    catch (...)
    {
        std::cerr << "Unknown exception\n";
        std::cerr.flush();
    }

    return 0;
}

