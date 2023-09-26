#include <iostream>
#include <fstream>
#include <vector>

class XMLNode
{
private:
    XMLNode *_prev = nullptr;
    XMLNode *_next = nullptr;
protected:
    XMLNode() { }
    XMLNode *_parent = nullptr;
public:
    virtual void serialize(std::ostream &os) const = 0;
    void next(XMLNode *next);
    void prev(XMLNode *prev) { _prev = prev; }
    void parent(XMLNode *parent) { _parent = parent; }
    XMLNode *prev() const { return _prev; }
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

    if (next() != nullptr)
        next()->serialize(os);
}

class XMLTag : public XMLNode
{
public:
    XMLNode *_firstChild = nullptr;
    std::string _name;
    XMLTag() { }
public:
    static XMLTag *create();
    void firstChild(XMLNode *child);
    void name(std::string s) { _name = s; }
    bool hasChild(XMLNode *child) { return false; }
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

void XMLTag::firstChild(XMLNode *child)
{
    _firstChild = child;
}

void XMLTag::serialize(std::ostream &os) const
{
#if 0
    std::cerr << "XMLTag::serialize\r\n";
#endif
    os << "<" << _name;

    if (_firstChild == nullptr)
        os.put('/');

    os.put('>');

    if (_firstChild)
    {
        _firstChild->serialize(os);
        os << "</" << _name << ">";
    }
}

class XMLDocument
{
private:
    XMLTag *_root = nullptr;
public:
    void parse(std::istream &is);
    void bogus();
    void serialize(std::ostream &os) const;
};

void XMLDocument::serialize(std::ostream &os) const
{
#if 0
    std::cerr << "XMLDocument::serialize\r\n";
#endif
    if (_root)
        _root->serialize(os);
}

void XMLDocument::bogus()
{
    _root = XMLTag::create();
    _root->name("person");
    _root->appendChild(XMLString::create("Jasper "));
    _root->appendChild(XMLString::create("ter "));
    _root->appendChild(XMLString::create("Weeme"));
#if 0
    XMLString *j = XMLString::create("Jasper ");
    XMLString *t = XMLString::create("ter ");
    XMLString *w = XMLString::create("Weeme");
    j->next(t);
    t->next(w);
    _root->firstChild(j);
    _root->lastChild()->prev()->serialize(std::cout);
    std::cout << "\r\n";
#endif
}

void XMLDocument::parse(std::istream &is)
{
    XMLTag *currentTag = nullptr;

    while (true)
    {
        int c = is.get();

        if (c == EOF)
            return;

        if (c == ' ' || c == '\r' || c == '\n')
            continue;

        if (c == '<')
        {
            c = is.peek();

            //XML Header
            if (c == '?')
            {
#if 0
                std::cerr << "XML Header\r\n";
#endif
                do
                {
                    c = is.get();
                }
                while (c != '>');

                continue;
            }

            //Close tag
            if (c == '/')
            {
                do
                {
                    c = is.get();
                }
                while (c != '>');

                if (currentTag->parent())
                {
                    currentTag = (XMLTag *)(currentTag->parent());
                    std::cerr << currentTag->_name << "\r\n";
                }

                continue;
            }

            //Open tag
            XMLTag *t = XMLTag::create();
            std::string name;

            while (true)
            {
                c = is.get();
                
                if (c == ' ' || c == '>')
                    break;

                name.push_back(c);
            }
            t->name(name);
            
            if (_root == nullptr)
            {
                _root = t;
            }
            else
            {
                std::cerr << t->_name << "\r\n";
                currentTag->appendChild(t);
            }

            currentTag = t;
            continue;
        }

        //String, geen tag gevonden
        std::string s;
        s.push_back(c);

        while (true)
        {
            c = is.peek();

            if (c == '<' || c == EOF)
                break;

            s.push_back(c);
            is.get();
        }

        XMLString *xs = XMLString::create(s);
        currentTag->appendChild(xs);
        
        std::cerr << s << "\r\n";

    }
}



int main(int argc, char **argv)
{
#if 1
    std::ifstream ifs(argv[1]);
    XMLDocument d;
    d.parse(ifs);
    ifs.close();
    d.serialize(std::cout);
    std::cout << "\r\n";
#endif

#if 0
    std::vector<XMLNode *> nodes;
    XMLString onzin("onzin");
    nodes.push_back(&onzin);

    for (std::vector<XMLNode *>::const_iterator it = nodes.cbegin(); it != nodes.cend(); ++it)
    {
        XMLNode *node = *it;
        node->serialize(std::cout);
    }
#endif

#if 0
    XMLDocument doc;
    doc.bogus();
    doc.serialize(std::cout);
    std::cout << "\r\n";
#endif
    return 0;
}


