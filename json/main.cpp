#include "json.h"
#include <iostream>

int main(int argc, char **argv)
{
    JSONRoot *root = new JSONRoot();
    parse(root, std::cin);
    root->serialize(std::cout);
    std::cout << "\r\n\r\n\r\n";
    delete root;
    return 0;
}


