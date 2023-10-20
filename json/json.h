#ifndef JSON_H
#define JSON_H
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

class JSONNumber : public JSONNode
{
private:
    std::string _n;
public:
    JSONNumber() { }
    JSONNumber(int n) { _n = std::to_string(n); }
    JSONNumber(std::string n) : _n(n) { }
    std::string value() const { return _n; }
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
    std::string key() const { return _key; }
    JSONNode *value() const { return _value; }
    void append(JSONNode *n) override { _value = n; }
    void serialize(std::ostream &os) override;
};

class JSONObject : public JSONNode
{
private:
    std::vector<JSONProperty *> _properties;
public:
    JSONObject() { }
    ~JSONObject();
    void appendProperty(JSONProperty *p) { _properties.push_back(p); }
    void append(std::string key, JSONNode *n) { appendProperty(new JSONProperty(key, n)); }
    void append(std::string key, std::string value) { append(key, new JSONString(value)); }
    std::vector<JSONProperty *>::iterator begin() { return _properties.begin(); }
    std::vector<JSONProperty *>::iterator end() { return _properties.end(); }
    JSONProperty *getProperty(std::string key) const;
    size_t size() const { return _properties.size(); }
    JSONProperty *at(size_t pos) const { return _properties.at(pos); }
    void removeProperty(JSONProperty *property);
    bool removeProperty(std::string key);
    void serialize(std::ostream &os) override;
};

class JSONArray : public JSONNode
{
private:
    std::vector<JSONNode *> _nodes;
public:
    ~JSONArray();
    std::vector<JSONNode *>::iterator begin() { return _nodes.begin(); }
    std::vector<JSONNode *>::iterator end() { return _nodes.end(); }
    void append(JSONNode *child);
    void serialize(std::ostream &os);
};

class JSONNull : public JSONNode
{
public:
    void serialize(std::ostream &os) { os << "null"; }
};

class Tokenizer
{
private:
    std::istream *_is;
public:
    Tokenizer(std::istream *is) : _is(is) { }
    std::string next();
};

typedef std::vector<std::string>::const_iterator cvecstrit;

void tokenize(std::vector<std::string> &tokens, std::istream &is);
void parse(cvecstrit it, cvecstrit end, JSONNode *parent);


#endif


