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
    void addAttribute(std::string name, std::string val);
    //bool hasChild(XMLNode *child) { return false; }
    XMLNode *lastChild() const;
    void remove() { }
    void removeAllChildren() { }
    void appendChild(XMLNode *node);
    void serialize(std::ostream &os) const;
    std::vector<XMLTag *> findTagsByTagname(std::string n);
};

std::vector<XMLTag *> XMLTag::findTagsByTagname(std::string n)
{
    std::vector<XMLTag *> ret;
    
    XMLNode *child = _firstChild;

    while (child != nullptr)
    {
        XMLTag *tag = dynamic_cast<XMLTag *>(child);

        if (tag)
        {
            if (tag->name().compare(n) == 0)
                ret.push_back(tag);

            std::vector<XMLTag *> sub = tag->findTagsByTagname(n);
            ret.insert(ret.end(), sub.begin(), sub.end());
        }

        child = child->next();
    }

    return ret;
}

void XMLTag::addAttribute(std::string name, std::string val)
{
    _attr.insert(std::make_pair(name, val));
}

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

void XMLDocument::parseTokens2(std::vector<XMLNode *> &nodes, std::vector<std::string> &tokens)
{
    int len = tokens.size();

    for (int i = 0; i < len; ++i)
    {
        //bewaar de tags voor later, doe eerst losse strings
        if (tokens[i].compare("<") != 0)
        {
            XMLString *s = XMLString::create(tokens[i]);
            nodes.push_back(s);
            continue;
        }

        ++i;
        std::string tag_name;

        //ignore headers and comments
        if (tokens[i].compare("?") == 0 || tokens[i].compare("!") == 0)
        {
            while (true)
            {
                ++i;

                if (tokens[i].compare(">") == 0)
                    break;
            }

            continue;
        }

        //first check for closing tag
        if (tokens[i].compare("/") == 0)
        {
            tag_name.append("/");
            ++i;
        }

        if (isalpha(tokens[i][0]) == false)
            throw "Mag niet!";

        tag_name.append(tokens[i]);
        ++i;
        XMLTag *tag = XMLTag::create();

        //attributes
        while (true)
        {
            if (isalpha(tokens[i][0]) == false)
                break;

            std::string attr = tokens[i];
            ++i;

            if (tokens[i].compare("=") != 0)
                throw "Specification mandates value for attribute";

            ++i;

            //trim leading and trailing quotes, single quotes not supported!
            std::string value = tokens[i].substr(1, tokens[i].length() - 2);
                
            tag->addAttribute(attr, value);
            ++i;
        }

        //check for self closing tag
        if (tokens[i].compare("/") == 0)
        {
            //we voegen een / toe aan de tagnaam als tip voor de treebuilder later
            tag_name.append("/");
            ++i;
        }

        if (tokens[i].compare(">") != 0)
            throw "No closing >";

        tag->name(tag_name);
        nodes.push_back(tag);
        continue;
    }
}

void XMLDocument::tokenize2(std::istream &is, std::vector<std::string> &tokens)
{
    while (true)
    {
        int c = is.get();

        if (c == EOF)
            break;

        std::string token;
        token.clear();

        while (c != '<' && c != EOF)
        {
            token.push_back(c);
            c = is.get();
        }

        if (token.size() > 0)
            tokens.push_back(token);

        if (c != '<')
            continue;

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

            if (c == '\"')
            {
                std::string token2;
                token2.push_back(c);

                do
                {
                    c = is.get();
                    token2.push_back(c);
                }
                while (c != '\"');

                tokens.push_back(token2);
                continue;
            }

            if (isalpha(c))
            {
                std::string token2;
                token2.push_back(c);

                while (true)
                {
                    c = is.peek();

                    if (isOneOf(">?!= ", c))
                    {
                        tokens.push_back(token2);
                        break;
                    }

                    c = is.get();
                    token2.push_back(c);
                }
            }
        }
    }
}

XMLTag *XMLDocument::_makeTree(std::vector<XMLNode *> &nodes)
{
    std::vector<XMLNode *>::iterator it = nodes.begin();
    XMLTag *root;

    //find root tag
    do
    {
        root = dynamic_cast<XMLTag *>(*it);
        ++it;
    }
    while (root == nullptr);

    XMLTag *currentTag = root;

    while (it != nodes.end())
    {
        XMLTag *tag = dynamic_cast<XMLTag *>(*it);

        if (tag)
        {
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

        //append XMLString
        if (currentTag)
            currentTag->appendChild(*it);

        ++it;
    }

    return root;
}

void XMLDocument::parse(std::istream &is)
{
    std::vector<std::string> tokens;
    tokenize2(is, tokens);
    std::vector<XMLNode *> nodes;
    parseTokens2(nodes, tokens);
    _root = _makeTree(nodes);
}

int main(int argc, char **argv)
{
    std::ifstream ifs(argv[1]);
    XMLDocument d;
    d.parse(ifs);
    ifs.close();
    d.serialize(std::cout);
    std::cout << "\r\n\r\n";

    XMLTag *root = d.root();
    std::vector<XMLTag *> tags = root->findTagsByTagname("a");

    for (auto tag : tags)
    {
        tag->serialize(std::cout);
        std::cout << "\r\n";
    }

    return 0;
}


