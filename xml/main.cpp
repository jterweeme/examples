#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>

class XMLNode
{
protected:
    XMLNode *_parent = nullptr;
    XMLNode *_prev = nullptr;
    XMLNode *_next = nullptr;
    XMLNode() { }
public:
    virtual void serialize(std::ostream &os) const = 0;
    void next(XMLNode *next);
    void prev(XMLNode *prev) { _prev = prev; }
    void parent(XMLNode *parent) { _parent = parent; }
    XMLNode *next() const { return _next; }
    XMLNode *parent() const { return _parent; }
    virtual void remove() = 0;
};

void XMLNode::next(XMLNode *next)
{
    next->prev(this);
    _next = next;
}

class XMLString : public XMLNode
{
private:
    std::string _s;
    XMLString(std::string s) : _s(s) { }
public:
    static XMLString *create(std::string s);
    void serialize(std::ostream &os) const;
    void remove();
};

XMLString *XMLString::create(std::string s)
{
    return new XMLString(s);
}

void XMLString::remove()
{
    delete this;
}

void XMLString::serialize(std::ostream &os) const
{
    os << _s;
}

class XMLTag : public XMLNode
{
public:
    XMLNode *_firstChild = nullptr;
    std::string _name;
    std::unordered_map<std::string, std::string> _attr;
    XMLTag() { }
public:
    static XMLTag *create();
    //void firstChild(XMLNode *child);
    void name(std::string s) { _name = s; }
    std::string name() { return _name; }
    //bool hasChild(XMLNode *child) { return false; }
    XMLNode *lastChild() const;
    void remove() { }
    void removeAllChildren() { }
    void appendChild(XMLNode *node);
    void serialize(std::ostream &os) const;
};

void XMLTag::appendChild(XMLNode *node)
{
    node->parent(this);

    if (!_firstChild)
    {
        _firstChild = node;
    }
    else
    {
        lastChild()->next(node);
    }
}

//prevent people from statically creating XMLTags
XMLTag *XMLTag::create()
{
    return new XMLTag;
}

XMLNode *XMLTag::lastChild() const
{
    if (_firstChild == nullptr)
        return nullptr;

    XMLNode *ret = _firstChild;

    while (ret->next())
        ret = ret->next();

    return ret;
}

#if 0
void XMLTag::firstChild(XMLNode *child)
{
    _firstChild = child;
}
#endif

void XMLTag::serialize(std::ostream &os) const
{
    os << "<" << _name;

    typedef std::unordered_map<std::string, std::string>::const_iterator umcit;

    for (umcit cit = _attr.cbegin(); cit != _attr.cend(); ++cit)
    {
        os << " " << cit->first << "=\"" << cit->second << "\"";
    }

    if (_firstChild == nullptr)
    {
        os << "/>";
        return;
    }

    os.put('>');
    XMLNode *current = _firstChild;

    while (true)
    {
        current->serialize(os);

        if (current->next() == nullptr)
            break;

        current = current->next();
    }

    os << "</" << _name << ">";
}

class XMLDocument
{
private:
    XMLTag *_root = nullptr;
    static void _tokenize(std::istream &is, std::vector<std::string> &tokens);
    static void _parseTag(XMLTag *tag, const std::string &s);
    static void _parseAttributes(std::unordered_map<std::string, std::string> &m, std::string &s);
    static void _makeNodes(std::vector<std::string> &tokens, std::vector<XMLNode *> &nodes);
    static XMLTag *_makeTree(std::vector<XMLNode *> &nodes);
public:
    XMLTag *root() { return _root; }
    void parse(std::istream &is);
    void serialize(std::ostream &os) const;
    static void tokenize2(std::istream &is, std::vector<std::string> &tokens);
    static void parseTokens2(std::vector<XMLNode *> &nodes, std::vector<std::string> &tokens);
    static bool isOneOf(std::string s, char c);
};

bool XMLDocument::isOneOf(std::string s, char c)
{
    return s.find(c) == -1 ? false : true;
}

void XMLDocument::serialize(std::ostream &os) const
{
    if (_root)
        _root->serialize(os);
}

#if 1
void XMLDocument::tokenize2(std::istream &is, std::vector<std::string> &tokens)
{
    std::string token;

    while (true)
    {
        int c = is.get();

        if (c == EOF)
            break;

        if (c == '<')
        {
            if (token.size() > 1)
            {
                tokens.push_back(token);
            }

            token.clear();
            tokens.push_back("<");

            while (true)
            {
                c = is.get();

                if (c == '>')
                {
                    tokens.push_back(">");
                    break;
                }

                if (isOneOf("?!=/", c))
                {
                    tokens.push_back(std::string(1, c));
                    continue;
                }

                if (c == 34)
                {
                    token.push_back(c);

                    do
                    {
                        c = is.get();
                        token.push_back(c);
                    }
                    while (c != 34);

                    tokens.push_back(token);
                    token.clear();
                    continue;
                }

                if (isalpha(c))
                {
                    token.push_back(c);

                    while (true)
                    {
                        c = is.peek();

                        if (isOneOf(">?!= ", c))
                        {
                            tokens.push_back(token);
                            token.clear();
                            break;
                        }

                        c = is.get();
                        token.push_back(c);
                    }
                    continue;
                }
            }
            continue;
        }

        token.push_back(c);
    }
}
#endif

void XMLDocument::_tokenize(std::istream &is, std::vector<std::string> &tokens)
{
    std::string token;

    while (true)
    {
        token.clear();
        int c = is.peek();

        if (c == EOF)
        {
            is.get();
            return;
        }

        if (c == ' ' || c == '\r' || c == '\n')
        {
            is.get();
            continue;
        }

        if (c == '<')
        {
            do
            {
                c = is.get();
                token.push_back(c);
            }
            while (c != '>');

            tokens.push_back(token);

            continue;
        }

        while (true)
        {
            c = is.peek();
            
            if (c == '<')
                break;

            c = is.get();
            token.push_back(c);
        }

        tokens.push_back(token);
    }
}

void XMLDocument::_makeNodes(std::vector<std::string> &tokens, std::vector<XMLNode *> &nodes)
{
    for (auto token : tokens)
    {
        //ignore XML header
        if (token[0] == '<' && token[1] == '?')
            continue;

        //ignore comments
        if (token[0] == '<' && token[1] == '!')
            continue;

        if (token[0] == '<')
        {
            XMLTag *tag = XMLTag::create();
            _parseTag(tag, token);
            nodes.push_back(tag);
            continue;
        }

        XMLString *str = XMLString::create(token);
        nodes.push_back(str);
        continue;
    }
}

XMLTag *XMLDocument::_makeTree(std::vector<XMLNode *> &nodes)
{
    std::vector<XMLNode *>::iterator it = nodes.begin();
    XMLTag *currentTag = dynamic_cast<XMLTag *>(*it);
    XMLTag *root = currentTag;
    ++it;

    while (it != nodes.end())
    {
        XMLTag *tag = dynamic_cast<XMLTag *>(*it);

        if (tag)
        {
            std::cerr << "XMLTag: " << tag->_name << "\r\n";

            if (tag->_name[0] == '/')
            {
                currentTag = dynamic_cast<XMLTag *>(currentTag->parent());
                ++it;
                continue;
            }

            //self closing
            if (tag->_name.back() == '/')
            {
                tag->_name.pop_back();
                currentTag->appendChild(tag);
                ++it;
                continue;
            }

            currentTag->appendChild(tag);
            currentTag = tag;
            ++it;
            continue;
        }

        if (dynamic_cast<XMLString *>(*it))
            std::cerr << "XMLString: \r\n";

        currentTag->appendChild(*it);

        ++it;
    }

    return root;
}

void XMLDocument::parse(std::istream &is)
{
    std::vector<std::string> tokens;
    _tokenize(is, tokens);
    std::vector<XMLNode *> nodes;
    _makeNodes(tokens, nodes);
    _root = _makeTree(nodes);
}

void XMLDocument::_parseAttributes(std::unordered_map<std::string, std::string> &m, std::string &s)
{
    std::string attr;
    std::string val;
    int len = s.length();

    for (int i = 0; i < len; ++i)
    {
        if (s[i] == ' ')
            continue;

        while (i < len)
        {
            if (s[i] == '=')
                break;

            attr.push_back(s[i]);
            ++i;
        }

        if (s[i] == '=')
        {
            ++i;
        }
    }
}

void XMLDocument::_parseTag(XMLTag *tag, const std::string &s)
{
    std::string name;
    int len = s.length();
    
    name.push_back(s[1]);
    int i;

    for (i = 2; i < len; ++i)
    {
        if (s[i] == ' ' || s[i] == '/' || s[i] == '>')
            break;

        name.push_back(s[i]);
    }

    if (s[i] == ' ')
    {
        ++i;
        std::string attributes;
        
        while (true)
        {
            if (s[i] == 34)
            {
                ++i;

                do
                {
                    attributes.push_back(s[i]);
                    ++i;
                }
                while (s[i] != 34);
            }
            if (s[i] == '/' || s[i] == '>')
                break;

            attributes.push_back(s[i]);
            ++i;
        }

        _parseAttributes(tag->_attr, attributes);
    }

    if (s[len - 2] == '/')
        name.push_back(s[len - 2]);

    tag->_name = name;
}

int main(int argc, char **argv)
{
    std::ifstream ifs(argv[1]);
    XMLDocument d;
    d.parse(ifs);
    ifs.close();
    d.serialize(std::cout);
    std::cout << "\r\n\r\n";
    //ifs.seekg(0, std::ios::beg);
    ifs.open(argv[1]);
    std::vector<std::string> tokens;
    d.tokenize2(ifs, tokens);
    
    for (auto token : tokens)
    {
        //std::cerr << token << "\r\n";
    }
    return 0;
}


