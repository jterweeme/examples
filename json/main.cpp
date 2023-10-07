#include <fstream>
#include <iostream>
#include <vector>

class JSONNode
{
public:
    virtual ~JSONNode() { }
    virtual void append(JSONNode *n) { std::cerr << "Append error\r\n"; throw "Error"; }
    virtual void serialize(std::ostream &os) { }
};

class JSONRoot : public JSONNode
{
private:
    JSONNode *_root;
public:
    ~JSONRoot() { delete _root; }
    JSONNode *root() const { return _root; }
    void append(JSONNode *n) override { _root = n; }
    void serialize(std::ostream &os) override { _root->serialize(os); }
};

class JSONString : public JSONNode
{
private:
    std::string _s;
public:
    JSONString() { }
    JSONString(std::string s) : _s(s) { }
    std::string s() const { return _s; }
    void serialize(std::ostream &os) override { os << "\"" << _s << "\""; }
};

class JSONBool : public JSONNode
{
private:
    bool _b;
public:
    JSONBool() { }
    JSONBool(bool b) : _b(b) { }
    void serialize(std::ostream &os) override;
};

void JSONBool::serialize(std::ostream &os)
{
    if (_b)
        os << "true";
    else
        os << "false";
}

class JSONNumber : public JSONNode
{
private:
    std::string _n;
public:
    JSONNumber() { }
    JSONNumber(int n) { _n = std::to_string(n); }
    JSONNumber(std::string n) : _n(n) { }
    void serialize(std::ostream &os) override { os << _n; }
};

class JSONProperty : public JSONNode
{
private:
    std::string _key;
    JSONNode *_value;
public:
    JSONProperty() { }
    JSONProperty(std::string key) : _key(key) { }
    JSONProperty(std::string key, JSONNode *val) : _key(key), _value(val) { }
    ~JSONProperty() { delete _value; }
    void append(JSONNode *n) override { _value = n; }
    void serialize(std::ostream &os) override;
};

void JSONProperty::serialize(std::ostream &os)
{
    os.put('\"');
    os << _key;
    os.put('\"');
    os.put(':');
    _value->serialize(os);
}

class JSONObject : public JSONNode
{
private:
    std::vector<JSONProperty *> _properties;
public:
    JSONObject() { }
    ~JSONObject() { for (auto property : _properties) delete property; }
    void appendProperty(JSONProperty *p) { _properties.push_back(p); }
    void append(std::string key, JSONNode *n) { appendProperty(new JSONProperty(key, n)); }
    void append(std::string key, std::string value) { append(key, new JSONString(value)); }
    void serialize(std::ostream &os) override;
};

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

class JSONArray : public JSONNode
{
private:
    std::vector<JSONNode *> _nodes;
public:
    ~JSONArray();
    void append(JSONNode *child);
    void serialize(std::ostream &os);
};

JSONArray::~JSONArray()
{
    for (auto node : _nodes)
        delete node;
}

class JSONNull : public JSONNode
{
public:
    void serialize(std::ostream &os) { os << "null"; }
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

        if (isOneOf("0123456789-.", c))
        {
            std::string token;
            token.push_back(c);

            while (true)
            {
                c = is.peek();

                if (isOneOf("0123456789-.", c) == false)
                    break;

                token.push_back(c);
                is.get();
            }

            tokens.push_back(token);
            continue;
        }

        if (isalpha(c))
        {
            std::string token;

            while (true)
            {
                token.push_back(c);
                c = is.peek();

                if (isalpha(c) == false)
                    break;

                c = is.get();
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

typedef std::vector<std::string>::const_iterator cvecstrit;

static void parse(cvecstrit it, cvecstrit end, JSONNode *parent)
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

            continue;
        }

        if (peek.compare(":") == 0)
        {
            std::string s = token.substr(1, token.length() - 2);
            JSONProperty *p = new JSONProperty(s);
            JSONObject *o = dynamic_cast<JSONObject *>(stack.back());
            o->appendProperty(p);
            stack.push_back(p);
            ++it;
            continue;
        }

        if (token.compare("null") == 0)
        {
            stack.back()->append(new JSONNull());
            continue;
        }

        if (token.compare("true") == 0)
        {
            stack.back()->append(new JSONBool(true));
            continue;
        }

        if (token.compare("false") == 0)
        {
            stack.back()->append(new JSONBool(false));
            continue;
        }

        if (token.compare("[") == 0)
        {
            JSONArray *a = new JSONArray();
            stack.back()->append(a);
            stack.push_back(a);
            continue;
        }

        if (token.compare("]") == 0)
        {
            stack.pop_back();
            continue;
        }
    
        if (token.compare("{") == 0)
        {
            JSONObject *o = new JSONObject();
            stack.back()->append(o);
            stack.push_back(o);
            continue;
        }

        if (token.compare("}") == 0)
        {
            stack.pop_back();
            stack.pop_back();
            continue;
        }

        if (token[0] == '\"')
        {
            std::string s = token.substr(1, token.length() - 2);
            stack.back()->append(new JSONString(s));
            continue;
        }

        if (isdigit(token[0]) || token[0] == '-')
        {
            stack.back()->append(new JSONNumber(token));
            continue;
        }

        std::cerr << token << "\r\n";
        throw "Onbekend token!";
    }
}

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
    std::cerr << "\r\n";
    delete root2;
#endif
    return 0;
}


