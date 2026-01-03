#include "lexer/lexer.hpp"

#include <cctype>
#include <map>
#include <string>

#include "common/common.hpp"

namespace tiger {

Lexer::Lexer(const std::string& source) : source_(source), pos_(0), line_(1), column_(1) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();

        if (isAtEnd()) break;

        // Check for comments
        if (peek() == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
            skipComment();
            continue;
        }

        Token token = scanToken();
        if (token.type != TokenType::EOF_TOKEN) {
            tokens.push_back(token);
        }
    }

    // Add EOF token
    tokens.emplace_back(TokenType::EOF_TOKEN, "", line_, column_);
    return tokens;
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source_[pos_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

bool Lexer::isAtEnd() const { return pos_ >= source_.size(); }

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    // Skip /* ... */ comments (supports nesting)
    int nesting = 0;
    while (!isAtEnd()) {
        if (peek() == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
            advance();  // '/'
            advance();  // '*'
            nesting++;
        } else if (peek() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
            advance();  // '*'
            advance();  // '/'
            nesting--;
            if (nesting == 0) break;
        } else {
            advance();
        }
    }
}

Token Lexer::scanToken() {
    char c = advance();

    // Identifiers and keywords
    if (isAlpha(c)) {
        // Put back the character
        pos_--;
        column_--;
        return scanIdentifier();
    }

    // Numbers
    if (isDigit(c)) {
        // Put back the character
        pos_--;
        column_--;
        return scanNumber();
    }

    // Strings
    if (c == '"') {
        return scanString();
    }

    // Operators and punctuation
    switch (c) {
        case '+':
            return Token(TokenType::PLUS, "+", line_, column_ - 1);
        case '-':
            return Token(TokenType::MINUS, "-", line_, column_ - 1);
        case '*':
            return Token(TokenType::TIMES, "*", line_, column_ - 1);
        case '/':
            return Token(TokenType::DIVIDE, "/", line_, column_ - 1);

        case ':':
            if (peek() == '=') {
                advance();
                return Token(TokenType::ASSIGN, ":=", line_, column_ - 2);
            }
            return Token(TokenType::COLON, ":", line_, column_ - 1);

        case ';':
            return Token(TokenType::SEMICOLON, ";", line_, column_ - 1);
        case ',':
            return Token(TokenType::COMMA, ",", line_, column_ - 1);
        case '.':
            return Token(TokenType::DOT, ".", line_, column_ - 1);

        case '(':
            return Token(TokenType::LPAREN, "(", line_, column_ - 1);
        case ')':
            return Token(TokenType::RPAREN, ")", line_, column_ - 1);
        case '[':
            return Token(TokenType::LBRACK, "[", line_, column_ - 1);
        case ']':
            return Token(TokenType::RBRACK, "]", line_, column_ - 1);
        case '{':
            return Token(TokenType::LBRACE, "{", line_, column_ - 1);
        case '}':
            return Token(TokenType::RBRACE, "}", line_, column_ - 1);

        case '=':
            return Token(TokenType::EQ, "=", line_, column_ - 1);
        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::LE, "<=", line_, column_ - 2);
            } else if (peek() == '>') {
                advance();
                return Token(TokenType::NEQ, "<>", line_, column_ - 2);
            }
            return Token(TokenType::LT, "<", line_, column_ - 1);

        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GE, ">=", line_, column_ - 2);
            }
            return Token(TokenType::GT, ">", line_, column_ - 1);

        case '&':
            return Token(TokenType::AND, "&", line_, column_ - 1);
        case '|':
            return Token(TokenType::OR, "|", line_, column_ - 1);

        default:
            throw LexicalError("Unexpected character: " + std::string(1, c), line_, column_ - 1);
    }
}

Token Lexer::scanIdentifier() {
    size_t start_line = line_;
    size_t start_column = column_;

    std::string identifier;
    while (isAlphaNumeric(peek())) {
        identifier += advance();
    }

    TokenType type = getKeywordType(identifier);
    return Token(type, identifier, start_line, start_column);
}

Token Lexer::scanNumber() {
    size_t start_line = line_;
    size_t start_column = column_;

    std::string number;
    while (isDigit(peek())) {
        number += advance();
    }

    int value = std::stoi(number);
    return Token(TokenType::INTEGER, number, start_line, start_column, value);
}

Token Lexer::scanString() {
    size_t start_line = line_;
    size_t start_column = column_;

    std::string str;
    while (peek() != '"' && !isAtEnd()) {
        char c = advance();
        if (c == '\\') {
            // Escape sequence
            if (!isAtEnd()) {
                c = advance();
                switch (c) {
                    case 'n':
                        str += '\n';
                        break;
                    case 't':
                        str += '\t';
                        break;
                    case '\\':
                        str += '\\';
                        break;
                    case '"':
                        str += '"';
                        break;
                    default:
                        str += c;
                        break;
                }
            }
        } else {
            str += c;
        }
    }

    if (isAtEnd()) {
        throw LexicalError("Unterminated string literal", start_line, start_column);
    }

    advance();  // Consume closing '"'
    return Token(TokenType::STRING, str, start_line, start_column);
}

bool Lexer::isDigit(char c) const { return std::isdigit(static_cast<unsigned char>(c)); }

bool Lexer::isAlpha(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c); }

TokenType Lexer::getKeywordType(const std::string& identifier) {
    static const std::map<std::string, TokenType> keywords = {
        {"type", TokenType::TYPE},   {"var", TokenType::VAR},     {"function", TokenType::FUNCTION},
        {"array", TokenType::ARRAY}, {"if", TokenType::IF},       {"then", TokenType::THEN},
        {"else", TokenType::ELSE},   {"while", TokenType::WHILE}, {"do", TokenType::DO},
        {"for", TokenType::FOR},     {"to", TokenType::TO},       {"let", TokenType::LET},
        {"in", TokenType::IN},       {"end", TokenType::END},     {"of", TokenType::OF},
        {"break", TokenType::BREAK}, {"nil", TokenType::NIL}};

    auto it = keywords.find(identifier);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::ID;
}

}  // namespace tiger
