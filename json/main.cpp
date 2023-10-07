#include "json.h"
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char **argv)
{
#if 0
    JSONArray *root = new JSONArray();
    root->append(new JSONNumber("1"));
    root->append(new JSONNumber("2"));
#if 0
    root->append(new JSONString("alpha"));
    root->append(new JSONString("bravo"));
    JSONObject *o = new JSONObject();
    o->append("naam", "Jasper");
    o->append("birthdate", "13/11/1988");
    root->append(o);a
#endif
    root->serialize(std::cerr);
    std::cerr << "\r\n";
    delete root;
#endif
#if 1
    std::vector<std::string> tokens;
    tokenize(tokens, std::cin);
#if 0
    for (auto token : tokens)
        std::cerr << token << "\r\n";
#endif
    JSONRoot *root2 = new JSONRoot();
    parse(tokens.cbegin(), tokens.cend(), root2);
    root2->serialize(std::cerr);
    std::cerr << "\r\n\r\n\r\n";

    JSONObject *o = dynamic_cast<JSONObject *>(root2->root());

    if (o)
    {
        o->removeProperty("buffers");
#if 0
        JSONProperty *buffers = o->getProperty("buffers");
    
        if (buffers)
            buffers->serialize(std::cerr);

        std::cerr << "\r\n";
#endif
    }

    root2->serialize(std::cerr);
    std::cerr << "\r\n\r\n";
    delete root2;
#endif
    return 0;
}


