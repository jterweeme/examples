#include "json.h"

void JSONBool::serialize(std::ostream &os)
{   
    if (_b)
        os << "true";
    else
        os << "false";
}
    
void JSONProperty::serialize(std::ostream &os)
{
    os.put('\"');
    os << _key;
    os.put('\"');
    os.put(':');
    _value->serialize(os);
}

void JSONObject::removeProperty(JSONProperty *property)
{   
    throw "to be implemented";
}

bool JSONObject::removeProperty(std::string key)
{   
    JSONProperty *prop = nullptr;
    std::vector<JSONProperty *>::iterator it = begin();

    while (it != end())
    {
        JSONProperty *tmp = *it++;

        if (tmp->key().compare(key) == 0)
            prop = tmp;
    }       

    if (prop)   
    {
        delete prop;
        _properties.erase(it);
        return true;
    }

    //object bevat geen property met gegeven key
    return false;
}

JSONObject::~JSONObject()
{
    for (auto property : _properties)
        delete property;
}

JSONProperty *JSONObject::getProperty(std::string key) const
{
    for (auto property : _properties)
    {
        if (property->key().compare(key) == 0)
            return property;
    }

    return nullptr;
}

void JSONObject::serialize(std::ostream &os)
{
    os.put('{');
    int len = _properties.size();
    int i;

    for (i = 0; i < len - 1; ++i)
    {
        _properties[i]->serialize(os);
        os.put(',');
    }

    if (i < len)
        _properties[i]->serialize(os);
    
    os.put('}');
}
    
JSONArray::~JSONArray()
{
    for (auto node : _nodes)
        delete node;
}

void JSONArray::serialize(std::ostream &os)
{
    os.put('[');
    int len = _nodes.size();
    int i;

    for (i = 0; i < len - 1; ++i)
    {
        _nodes[i]->serialize(os);
        os.put(',');
    }

    if (i < len)
        _nodes[i]->serialize(os);

    os.put(']');
}

void JSONArray::append(JSONNode *child)
{
    _nodes.push_back(child);
}

static bool isOneOf(std::string s, char c)
{
    return s.find(c) == -1 ? false : true;
}

void tokenize(std::vector<std::string> &tokens, std::istream &is)
{           
    while (true)
    {       
        int c = is.get();

        if (c == EOF)
            break;

        if (isOneOf("[]{}:,", c))
        {
            tokens.push_back(std::string(1, c));
            continue;
        }
        
        //numbers -0.2e3, true, false, null
        if (isOneOf("0123456789-.aeflnrstu", c))
        {
            std::string token;
            token.push_back(c);

            while (true)
            {
                c = is.peek();

                if (isOneOf("0123456789-.aeflnrstu", c) == false)
                    break;

                token.push_back(c);
                is.get();
            }
    
            tokens.push_back(token);
            continue;
        }

        if (c == '\"')
        {
            std::string token;
            token.push_back(c);

            do
            {
                c = is.get();
                token.push_back(c);
            }
            while (c != '\"');

            tokens.push_back(token);
            continue;
        }
    }
}

void parse(cvecstrit it, cvecstrit end, JSONNode *parent)
{
    std::vector<JSONNode *> stack;
    stack.push_back(parent);
    std::string peek;

    while (it != end)
    {
        JSONProperty *prop = dynamic_cast<JSONProperty *>(stack.back());
        std::string token = *it++;
        peek.clear();

        if (it != end)
            peek = *it;

        if (token.compare(",") == 0)
        {
            if (prop)
                stack.pop_back();
        }
        else if (peek.compare(":") == 0)
        {
            std::string s = token.substr(1, token.length() - 2);
            JSONProperty *p = new JSONProperty(s);
            JSONObject *o = dynamic_cast<JSONObject *>(stack.back());
            o->appendProperty(p);
            stack.push_back(p);
            ++it;
        }
        else if (token.compare("null") == 0)
        {
            stack.back()->append(new JSONNull());
        }
        else if (token.compare("true") == 0)
        {
            stack.back()->append(new JSONBool(true));
        }
        else if (token.compare("false") == 0)
        {
            stack.back()->append(new JSONBool(false));
        }
        else if (token.compare("[") == 0)
        {
            JSONArray *a = new JSONArray();
            stack.back()->append(a);
            stack.push_back(a);
        }
        else if (token.compare("]") == 0)
        {
            stack.pop_back();
        }
        else if (token.compare("{") == 0)
        {
            JSONObject *o = new JSONObject();
            stack.back()->append(o);
            stack.push_back(o);
        }
        else if (token.compare("}") == 0)
        {
            //in case of empty object
            JSONObject *test = dynamic_cast<JSONObject *>(stack.back());

            if (test)
            {
                stack.pop_back();
            }
            else
            {
                stack.pop_back();
                stack.pop_back();
            }
        }
        else if (token[0] == '\"')
        {
            std::string s = token.substr(1, token.length() - 2);
            stack.back()->append(new JSONString(s));
        }
        else if (isdigit(token[0]) || token[0] == '-')
        {
            stack.back()->append(new JSONNumber(token));
        }
        else
        {
            std::cerr << "Onbekend token: " << token << "\r\n";
            throw "Onbekend token!";
        }
    }
}

