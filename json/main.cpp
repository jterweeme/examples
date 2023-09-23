#include <fstream>
#include <iostream>

class JSONDocument
{
    
};

class JSONNode
{
    JSONNode *next;
};

class JSONObject : public JSONNode
{
    
};

class JSONArray : public JSONNode
{
};

struct Token
{
    enum class TYPE
    {
        CURLY_OPEN,
        CURLY_CLOSE,
        COLON,
        COMMA,
        NUMBER,
        NULL_TYPE,
        STRING
    };
    TYPE type;
    std::string value;
};

template <typename T> class Generator
{
public:
    virtual bool hasNext() = 0;
    virtual T next() = 0;
};

class Tokenizer : public Generator<Token>
{
    std::istream *_is;
    bool _token = false;
    Token _next;
public:
    Tokenizer(std::istream *is);
    bool hasNext();
    Token next();
};

Tokenizer::Tokenizer(std::istream *is) : _is(is)
{
}

bool Tokenizer::hasNext()
{
    if (_token == true)
        return true;

    _next.value = "";

    while (true)
    {
        int c = _is->get();

        if (c == EOF)
            return false;

        if (c == ' ')
            continue;

        if (c == '{')
        {
            _next.type = Token::TYPE::CURLY_OPEN;
            _next.value = "";
            return true;
        }

        if (c == '}')
        {
            _next.type = Token::TYPE::CURLY_CLOSE;
            _next.value = "";
            return true;
        }

        if (c == ':')
        {
            _next.type = Token::TYPE::COLON;
            _next.value = "";
            return true;
        }

        if (c == ',')
        {
            _next.type = Token::TYPE::COMMA;
            _next.value = "";
            return true;
        }

        if (isdigit(c))
        {
            _next.type = Token::TYPE::NUMBER;
            _next.value.push_back(c);
            
            while (true)
            {
                int c = _is->peek();
                
                if (isdigit(c) == false)
                    break;

                _is->get();
                _next.value.push_back(c);
            }
            return true;
        }

        if (c == 34)
        {
            _next.type = Token::TYPE::STRING;

            while (true)
            {
                int c = _is->get();

                if (c == 34)
                    break;
                
                _next.value.push_back(c);
            }
            return true;
        }

        if (c == 'n')
        {
            _is->get();
            _is->get();
            _is->get();
            _next.type = Token::TYPE::NULL_TYPE;
            _next.value = "";
            return true;
        }

    }
    return false;
}

Token Tokenizer::next()
{
    _token = false;
    return _next;
}

int main(int argc, char **argv)
{
    std::ifstream ifs(argv[1]);
    Tokenizer t(&ifs);
    
    while (t.hasNext())
    {
        Token tok = t.next();

        if (tok.type == Token::TYPE::CURLY_OPEN)
            std::cout << "{" << "\r\n";

        if (tok.type == Token::TYPE::CURLY_CLOSE)
            std::cout << "}" << "\r\n";

        if (tok.type == Token::TYPE::COLON)
            std::cout << ":" << "\r\n";

        if (tok.type == Token::TYPE::COMMA)
            std::cout << "," << "\r\n";

        if (tok.type == Token::TYPE::STRING || tok.type == Token::TYPE::NUMBER)
            std::cout << tok.value << "\r\n";

        if (tok.type == Token::TYPE::NULL_TYPE)
            std::cout << "null" << "\r\n";

    }
    ifs.close();
    return 0;
}


