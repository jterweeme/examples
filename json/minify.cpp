#include "json.h"
#include <iostream>

int main()
{
    Tokenizer tokenizer(&std::cin);

    while (true)
    {
        std::string token = tokenizer.next();

        if (token.size() == 0)
            break;

        std::cout << token;
    }

    std::cout << "\r\n";
    return 0;
}


