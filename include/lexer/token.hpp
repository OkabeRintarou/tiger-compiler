#ifndef TIGER_TOKEN_HPP
#define TIGER_TOKEN_HPP

#include <string>

namespace tiger {

// Token types
enum class TokenType {
    // Keywords
    TYPE,
    VAR,
    FUNCTION,
    ARRAY,
    IF,
    THEN,
    ELSE,
    WHILE,
    DO,
    FOR,
    TO,
    LET,
    IN,
    END,
    OF,
    BREAK,
    NIL,

    // Identifiers and literals
    ID,
    INTEGER,
    STRING,

    // Operators and punctuation
    PLUS,
    MINUS,
    TIMES,
    DIVIDE,
    EQ,
    NEQ,
    LT,
    GT,
    LE,
    GE,
    AND,
    OR,
    ASSIGN,
    COLON,
    SEMICOLON,
    COMMA,
    DOT,
    LPAREN,
    RPAREN,
    LBRACK,
    RBRACK,
    LBRACE,
    RBRACE,

    // Special
    EOF_TOKEN,

    // Error
    ERROR
};

// Token class
class Token {
public:
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    int integer_value;  // For INTEGER tokens

    Token(TokenType t, const std::string& lex, int ln, int col)
        : type(t), lexeme(lex), line(ln), column(col), integer_value(0) {}

    Token(TokenType t, const std::string& lex, int ln, int col, int val)
        : type(t), lexeme(lex), line(ln), column(col), integer_value(val) {}
};

// Convert token type to string (for debugging)
std::string tokenTypeToString(TokenType type);

}  // namespace tiger

#endif  // TIGER_TOKEN_HPP
