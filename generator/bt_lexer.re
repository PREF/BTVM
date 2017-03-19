#include "bt_lexer.h"
#include "bt_parser.h"

#define TOKEN_STRING string(start, this->_cursor - start)
#define TOKEN_STRING_LITERAL string(start + 1, this->_cursor - (start + 2))

#define UNKNOWN_TOKEN cout << "Unknown token '" << TOKEN_STRING << "' near line " << currentline << endl; \
                              continue;

#define TOKENIZE(token_type) token.value = TOKEN_STRING; \
                             token.type = token_type;    \
                             token.line = currentline;   \
                             tokens.push_back(token);    \
                             continue;

#define TOKENIZE_STRING_LITERAL(token_type) token.value = TOKEN_STRING_LITERAL; \
                                            token.type = token_type;    \
                                            token.line = currentline;   \
                                            tokens.push_back(token);    \
                                            continue;

BTLexer::BTLexer(const char* s): _cursor(s), _start(NULL), _marker(NULL)
{
}

string BTLexer::getString(const char* start, const char* end) const
{
    return string(start, end - start);
}

list<BTLexer::Token> BTLexer::lex()
{
    list<BTLexer::Token> tokens;
    unsigned int currentline = 0;

    for(; ;)
    {
        Token token;
        const char* start = this->_cursor;

        /*!re2c
            re2c:define:YYCTYPE  = char;
            re2c:define:YYCURSOR = this->_cursor;
            re2c:define:YYMARKER = this->_marker;
            re2c:yyfill:enable = 0;

            LINE_FEED      = [\n];
            WHITESPACE     = [\r\t ]+;
            LITERAL_REAL   = [0-9]*"."[0-9]* | [0-9]+".";
            LITERAL_OCT    = "0"[0-7]*;
            LITERAL_DEC    = [1-9][0-9]*;
            LITERAL_HEX    = "0x"[0-9a-fA-F]+;
            LITERAL_STRING = [L]?"\""("."|[^\"])*"\"";
            IDENTIFIER     = [a-zA-Z_][a-zA-Z_0-9]*;
            LINE_COMMENT   = "\x2f\x2f"[^\n]*"\n";
            MULTI_COMMENT  = "\x2f\x2a"([^*]|("*"[^/]))*"\x2a\x2f";

            "\x00"                     { break; }
            LINE_FEED  | LINE_COMMENT  { currentline++; continue; }
            WHITESPACE | MULTI_COMMENT { continue; }

            "(" { TOKENIZE(BTT_O_ROUND)   }
            ")" { TOKENIZE(BTT_C_ROUND)   }
            "[" { TOKENIZE(BTT_O_SQUARE)  }
            "]" { TOKENIZE(BTT_C_SQUARE)  }
            "{" { TOKENIZE(BTT_O_CURLY)   }
            "}" { TOKENIZE(BTT_C_CURLY)   }
            "," { TOKENIZE(BTT_COMMA)     }
            "." { TOKENIZE(BTT_DOT)       }
            ";" { TOKENIZE(BTT_SEMICOLON) }
            ":" { TOKENIZE(BTT_COLON)     }
            "=" { TOKENIZE(BTT_ASSIGN)    }
            "?" { TOKENIZE(BTT_QUESTION)  }

            "&"   { TOKENIZE(BTT_BIN_AND) }
            "|"   { TOKENIZE(BTT_BIN_OR)  }
            "^"   { TOKENIZE(BTT_BIN_XOR) }
            "~"   { TOKENIZE(BTT_BIN_NOT) }

            "&&"  { TOKENIZE(BTT_LOG_AND) }
            "||"  { TOKENIZE(BTT_LOG_OR)  }
            "!"   { TOKENIZE(BTT_LOG_NOT) }

            "=="  { TOKENIZE(BTT_EQ) }
            "!="  { TOKENIZE(BTT_NE) }
            "<"   { TOKENIZE(BTT_LT) }
            ">"   { TOKENIZE(BTT_GT) }
            "<="  { TOKENIZE(BTT_LE) }
            ">="  { TOKENIZE(BTT_GE) }

            "+="  { TOKENIZE(BTT_ADD_ASSIGN)    }
            "-="  { TOKENIZE(BTT_SUB_ASSIGN)    }
            "*="  { TOKENIZE(BTT_MUL_ASSIGN)    }
            "/="  { TOKENIZE(BTT_DIV_ASSIGN)    }
            "&="  { TOKENIZE(BTT_AND_ASSIGN)    }
            "|="  { TOKENIZE(BTT_OR_ASSIGN)     }
            "^="  { TOKENIZE(BTT_XOR_ASSIGN)    }
            "<<=" { TOKENIZE(BTT_LS_ASSIGN)     }
            ">>=" { TOKENIZE(BTT_RS_ASSIGN)     }

            "++"  { TOKENIZE(BTT_INC) }
            "--"  { TOKENIZE(BTT_DEC) }

            "+"   { TOKENIZE(BTT_ADD) }
            "-"   { TOKENIZE(BTT_SUB) }
            "*"   { TOKENIZE(BTT_MUL) }
            "/"   { TOKENIZE(BTT_DIV) }
            "%"   { TOKENIZE(BTT_MOD) }
            "<<"  { TOKENIZE(BTT_LSL) }
            ">>"  { TOKENIZE(BTT_LSR) }

            "void"                                                                   { TOKENIZE(BTT_VOID)     }
            "bool"                                                                   { TOKENIZE(BTT_BOOL)     }
            "string"																 { TOKENIZE(BTT_STRING)   }
            "wstring"																 { TOKENIZE(BTT_WSTRING)  }
            "wchar_t"																 { TOKENIZE(BTT_WCHAR)    }
            "char"   | "CHAR"                                                        { TOKENIZE(BTT_CHAR)     }
            "byte"   | "BYTE"                                                        { TOKENIZE(BTT_BYTE)     }
            "uchar"  | "UCHAR"                                                       { TOKENIZE(BTT_UCHAR)    }
            "ubyte"  | "UBYTE"                                                       { TOKENIZE(BTT_UBYTE)    }
            "short"  | "int16"  | "SHORT"  | "INT16"                                 { TOKENIZE(BTT_SHORT)    }
            "ushort" | "uint16" | "USHORT" | "UINT16" | "WORD"                       { TOKENIZE(BTT_USHORT)   }
            "int"    | "int32"  | "long"   | "INT"    | "INT32"  | "LONG"            { TOKENIZE(BTT_INT32)    }
            "uint"   | "uint32" | "ulong"  | "UINT"   | "UINT32" | "ULONG" | "DWORD" { TOKENIZE(BTT_UINT32)   }
            "int64"  | "quad"   | "INT64"  | "QUAD"   | "__int64"                    { TOKENIZE(BTT_INT64)    }
            "uint64" | "uquad"  | "UINT64" | "UQUAD"  | "QWORD"  | "__uint64"        { TOKENIZE(BTT_UINT64)   }
            "hfloat" | "HFLOAT"                                                      { TOKENIZE(BTT_HFLOAT)   }
            "float"  | "FLOAT"                                                       { TOKENIZE(BTT_FLOAT)    }
            "double" | "DOUBLE"                                                      { TOKENIZE(BTT_DOUBLE)   }
            "time_t"                                                                 { TOKENIZE(BTT_TIME)     }
            "DOSDATE"                                                                { TOKENIZE(BTT_DOSDATE)  }
            "DOSTIME"                                                                { TOKENIZE(BTT_DOSTIME)  }
            "OLETIME"                                                                { TOKENIZE(BTT_OLETIME)  }
            "FILETIME"                                                               { TOKENIZE(BTT_FILETIME) }

            "const"     { TOKENIZE(BTT_CONST)    }
            "local"     { TOKENIZE(BTT_LOCAL)    }
            "break"     { TOKENIZE(BTT_BREAK)    }
            "continue"  { TOKENIZE(BTT_CONTINUE) }
            "sizeof"    { TOKENIZE(BTT_SIZEOF)   }
            "typedef"   { TOKENIZE(BTT_TYPEDEF)  }
            "enum"      { TOKENIZE(BTT_ENUM)     }
            "struct"    { TOKENIZE(BTT_STRUCT)   }
            "union"     { TOKENIZE(BTT_UNION)    }
            "return"    { TOKENIZE(BTT_RETURN)   }
            "case"      { TOKENIZE(BTT_CASE)     }
            "default"   { TOKENIZE(BTT_DEFAULT)  }
            "for"       { TOKENIZE(BTT_FOR)      }
            "if"        { TOKENIZE(BTT_IF)       }
            "else"      { TOKENIZE(BTT_ELSE)     }
            "switch"    { TOKENIZE(BTT_SWITCH)   }
            "while"     { TOKENIZE(BTT_WHILE)    }
            "do"        { TOKENIZE(BTT_DO)       }
            "const"     { TOKENIZE(BTT_CONST)    }
            "signed"    { TOKENIZE(BTT_SIGNED)   }
            "unsigned"  { TOKENIZE(BTT_UNSIGNED) }
            "true"      { TOKENIZE(BTT_TRUE)     }
            "false"     { TOKENIZE(BTT_FALSE)    }

            IDENTIFIER     { TOKENIZE(BTT_IDENTIFIER)                    }
            LITERAL_REAL   { TOKENIZE(BTT_LITERAL_REAL)                  }
            LITERAL_OCT    { TOKENIZE(BTT_LITERAL_OCT)                   }
            LITERAL_DEC    { TOKENIZE(BTT_LITERAL_DEC)                   }
            LITERAL_HEX    { TOKENIZE(BTT_LITERAL_HEX)                   }
            LITERAL_STRING { TOKENIZE_STRING_LITERAL(BTT_LITERAL_STRING) }

            * { UNKNOWN_TOKEN }
         */
    }

    return tokens;
}
 
