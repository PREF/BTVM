#ifndef BT_LEXER_H
#define BT_LEXER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <list>

using namespace std;

class BTLexer
{
    public:
        struct Token { int type; unsigned int line; std::string value; };

    public:
        BTLexer(const char* s);
        list<BTLexer::Token> lex();

    private:
        string getString(const char* start, const char* end) const;

    private:
        const char* _cursor;
        const char* _start;
        const char* _marker;
};

#endif // FLINI_LEXER_H
