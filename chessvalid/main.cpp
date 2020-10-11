#include "game.h"
#include <iostream>

int main()
{
    Game game;

    try
    {
        game.reset();
        std::cout << game.asciiBoard() << "\n\n";
        std::cout << game.writeForsythe() << "\n";
        std::cout.flush();
    }
    catch (...)
    {

    }

    return 0;
}

