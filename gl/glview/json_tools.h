#ifndef JSON_TOOLS_H
#define JSON_TOOLS_H

#include "json.h"

static void ParseStringProperty(std::string &s, JSONObject *o, std::string prop, bool req)
{     
    JSONProperty *p = o->getProperty(prop);
        
    if (p == nullptr)
    {     
        if (req)
            throw "Cannot find property";
            
        return;               
    }                  
          
    JSONString *str = dynamic_cast<JSONString *>(p->value());
    s = str->s();
}

template <typename T>
static void ParseIntegerProperty(T &i, JSONObject *o, std::string prop, bool req)
{
    JSONProperty *p = o->getProperty(prop);
    
    if (p == nullptr)
    {
        if (req)              
            throw "Cannot find required property";

        return;
    }

    JSONNumber *nr = dynamic_cast<JSONNumber *>(p->value());

    if (nr == nullptr)
    {
        if (req) 
            throw "Not the right type";

        return;
    }

    i = T(stoi(nr->value()));
}

static void ParseDoubleArray(std::vector<double> &v, JSONArray *a)
{
    for (JSONNode *node : *a)
    {
        JSONNumber *nr = dynamic_cast<JSONNumber *>(node);
        v.push_back(std::stod(nr->value()));
    }
}

static void
ParseDoubleArrayProperty(std::vector<double> &v, JSONObject *o, std::string key, bool req)
{
    JSONProperty *p = dynamic_cast<JSONProperty *>(o->getProperty(key));

    if (p == nullptr)
    {
        if (req)
            throw "No such property";

        return;
    }

    JSONArray *a = dynamic_cast<JSONArray *>(p->value());

    if (a == nullptr)
    {
        if (req)
            throw "Property not an array";

        return;
    }

    ParseDoubleArray(v, a);
}

static void ParseIntegerArray(std::vector<int> &v, JSONArray *a)
{
    for (JSONNode *node : *a)
    {
        JSONNumber *nr = dynamic_cast<JSONNumber *>(node);
        v.push_back(stoi(nr->value()));
    }
}

static void
ParseIntegerArrayProperty(std::vector<int> &v, JSONObject *o, std::string key, bool req)
{
    JSONProperty *p = dynamic_cast<JSONProperty *>(o->getProperty(key));

    if (p == nullptr)
    {
        if (req)
            throw "No such property";

        return;
    }

    JSONArray *a = dynamic_cast<JSONArray *>(p->value());

    if (a == nullptr)
    {
        if (req)
            throw "Property not an array";

        return;
    }

    ParseIntegerArray(v, a);
}


#endif


