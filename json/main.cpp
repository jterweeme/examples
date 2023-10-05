#include <fstream>
#include <iostream>
#include <vector>
#include <map>

class JSONNode
{
public:
    virtual void serialize(std::ostream &os) { }
};

class JSONString : public JSONNode
{
private:
    std::string _s;
public:
    JSONString() { }
    JSONString(std::string s) : _s(s) { }
    void serialize(std::ostream &os) { os << "\"" << _s << "\""; }
};

class JSONNumber : public JSONNode
{
private:
    int _n;
public:
    JSONNumber() { }
    JSONNumber(int n) : _n(n) { }
    void serialize(std::ostream &os) { os << _n; }
};

class JSONObject : public JSONNode
{
private:
    std::vector<std::pair<std::string, JSONNode *>> _map;
public:
    void append(std::pair<std::string, JSONNode *> pair);
    void append(std::string s, JSONNode *n);
    void append(std::string s, std::string v) { append(s, new JSONString(v)); }
    void serialize(std::ostream &os);
};

void JSONObject::append(std::pair<std::string, JSONNode *> pair)
{
    _map.push_back(pair);
}

void JSONObject::append(std::string s, JSONNode *n)
{
    append(std::pair<std::string, JSONNode *>(s, n));
}

void JSONObject::serialize(std::ostream &os)
{
    os.put('{');
    int len = _map.size();
    int i;

    for (i = 0; i < len; ++i)
    {
        os.put('\"');
        os << _map[i].first;
        os.put('\"');
        os.put(':');
        _map[i].second->serialize(os);

        if (i < len - 1)
            os.put(',');
    }

    os.put('}');
}

class JSONArray : public JSONNode
{
private:
    std::vector<JSONNode *> _nodes;
public:
    void appendChild(JSONNode *child);
    void serialize(std::ostream &os);
};

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

    _nodes[i]->serialize(os);

    os.put(']');
}

void JSONArray::appendChild(JSONNode *child)
{
    _nodes.push_back(child);
}

static bool isOneOf(std::string s, char c)
{
    return s.find(c) == -1 ? false : true;
}

static void tokenize(std::vector<std::string> &tokens, std::istream &is)
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

        if (isdigit(c))
        {
            std::string token;
            token.push_back(c);

            while (true)
            {
                c = is.peek();

                if (isdigit(c) == false)
                    break;

                token.push_back(c);
                is.get();
            }

            tokens.push_back(token);
            continue;
        }

        if (c == 'n')
        {
            while (true)
            {
                c = is.peek();

                if (isalpha(c) == false)
                    break;

                is.get();
            }
            tokens.push_back("null");
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

int main(int argc, char **argv)
{
    JSONArray *root = new JSONArray();
    root->appendChild(new JSONNumber(1));
    root->appendChild(new JSONNumber(2));
    root->appendChild(new JSONString("alpha"));
    root->appendChild(new JSONString("bravo"));
    JSONObject *o = new JSONObject();
    o->append("naam", "Jasper");
    o->append("birthdate", "13/11/1988");
    root->appendChild(o);
    root->serialize(std::cerr);
    std::cerr << "\r\n";
    std::vector<std::string> tokens;
    tokenize(tokens, std::cin);
    
    for (auto token : tokens)
    {
        std::cerr << token << "\r\n";
    }
    return 0;
}


