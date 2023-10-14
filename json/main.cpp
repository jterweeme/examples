#include "json.h"
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char **argv)
{
#if 0
    JSONObject *root = new JSONObject();
    JSONProperty *prop = new JSONProperty("animations");
    root->appendProperty(prop);
    JSONArray *a = new JSONArray();
    prop->append(a);
    a->append(new JSONObject());
    JSONObject *o = new JSONObject();
    a->append(o);
    prop = new JSONProperty("extensions");
    o->appendProperty(prop);
    prop->append(new JSONObject());
    o = new JSONObject();
    a->append(o);
    prop = new JSONProperty("extensions");
    o->appendProperty(prop);
    o = new JSONObject();
    prop->append(o);
    prop = new JSONProperty("ASOBO_material_invisible");
    o->appendProperty(prop);
    prop->append(new JSONObject());
    prop = new JSONProperty("ASOBO_tags");
    o->appendProperty(prop);
    o = new JSONObject();
    prop->append(o);
    prop = new JSONProperty("tags");
    o->appendProperty(prop);
    a = new JSONArray();
    prop->append(a);
    a->append(new JSONString("Collision"));
    root->serialize(std::cout);
    std::cout << "\r\n\r\n";
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
    root2->serialize(std::cout);
    std::cout << "\r\n\r\n\r\n";
    delete root2;
#endif
    return 0;
}


