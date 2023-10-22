#include "json.h"
#include <iostream>

int main()
{
    while (true)
    {
        std::string token = next_token(std::cin);

        if (token.size() == 0)
            break;

        std::cout << token;
    }

    std::cout << "\r\n";
    return 0;
}


