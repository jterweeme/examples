#include <iostream>
#include <fstream>
#include <vector>

class XMLNode
{
protected:
    XMLNode *_next = nullptr;
public:
    virtual void serialize(std::ostream &os) const = 0;
    void next(XMLNode *next) { _next = next; }
    XMLNode *next() const { return _next; }
};

class XMLString : public XMLNode
{
    std::string _s;
public:
    XMLString(std::string s) : _s(s) { }
    void serialize(std::ostream &os) const;
};

void XMLString::serialize(std::ostream &os) const
{
    os << _s;

    if (next() != nullptr)
        next()->serialize(os);
}

class XMLTag : public XMLNode
{
    XMLNode *_child = nullptr;
    std::string _name;
public:
    void child(XMLNode *child) { _child = child; }
    void name(std::string s) { _name = s; }
    void serialize(std::ostream &os) const;
};

void XMLTag::serialize(std::ostream &os) const
{
    os << "<" << _name;

    if (_child == nullptr)
        os.put('/');

    os.put('>');

    if (_child)
    {
        _child->serialize(os);
        os << "</" << _name << ">";
    }
}

class XMLDocument
{
    XMLTag *_root;
public:
    void parse(std::istream &is);
    void bogus();
    void serialize(std::ostream &os) const { _root->serialize(os); }
};

void XMLDocument::bogus()
{
    _root = new XMLTag;
    _root->name("person");
    XMLNode *j = new XMLString("Jasper");
    XMLNode *tw = new XMLString(" ter Weeme");
    j->next(tw);
    _root->child(j);
}

void XMLDocument::parse(std::istream &is)
{
    while (true)
    {
        int c = is.get();

        if (c == EOF)
            return;

        if (c == ' ')
            continue;

        if (c == '<')
        {
            c = is.get();

            if (c == '?')
            {
                std::cerr << "XML Header\r\n";
                
                do
                {
                    c = is.get();
                }
                while (c != '>');

                continue;
            }
            XMLTag *t = new XMLTag;
        }
    }
}



int main(int argc, char **argv)
{
#if 0
    std::ifstream ifs(argv[1]);
    XMLDocument d;
    d.parse(ifs);
    ifs.close();
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
    XMLDocument d;
    d.bogus();
    d.serialize(std::cout);
    std::cout << "\r\n";

    return 0;
}


