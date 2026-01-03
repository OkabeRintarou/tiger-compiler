#ifndef TIGER_LEXER_HPP
#define TIGER_LEXER_HPP

#include <string>
#include <vector>

#include "lexer/token.hpp"

namespace tiger {

class Lexer {
   public:
    explicit Lexer(const std::string& source);

    // Tokenize the entire source and return a list of tokens
    std::vector<Token> tokenize();

   private:
    std::string source_;
    size_t pos_;
    int line_;
    int column_;

    // Helper methods
    char peek() const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespace();
    void skipComment();

    Token scanToken();
    Token scanIdentifier();
    Token scanNumber();
    Token scanString();

    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;

    TokenType getKeywordType(const std::string& identifier);
};

}  // namespace tiger

#endif  // TIGER_LEXER_HPP
