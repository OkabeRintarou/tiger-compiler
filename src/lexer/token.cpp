#include "lexer/token.hpp"

#include <string>

namespace tiger {

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::TYPE:
            return "TYPE";
        case TokenType::VAR:
            return "VAR";
        case TokenType::FUNCTION:
            return "FUNCTION";
        case TokenType::ARRAY:
            return "ARRAY";
        case TokenType::IF:
            return "IF";
        case TokenType::THEN:
            return "THEN";
        case TokenType::ELSE:
            return "ELSE";
        case TokenType::WHILE:
            return "WHILE";
        case TokenType::DO:
            return "DO";
        case TokenType::FOR:
            return "FOR";
        case TokenType::TO:
            return "TO";
        case TokenType::LET:
            return "LET";
        case TokenType::IN:
            return "IN";
        case TokenType::END:
            return "END";
        case TokenType::OF:
            return "OF";
        case TokenType::BREAK:
            return "BREAK";
        case TokenType::NIL:
            return "NIL";
        case TokenType::ID:
            return "ID";
        case TokenType::INTEGER:
            return "INTEGER";
        case TokenType::STRING:
            return "STRING";
        case TokenType::PLUS:
            return "PLUS";
        case TokenType::MINUS:
            return "MINUS";
        case TokenType::TIMES:
            return "TIMES";
        case TokenType::DIVIDE:
            return "DIVIDE";
        case TokenType::EQ:
            return "EQ";
        case TokenType::NEQ:
            return "NEQ";
        case TokenType::LT:
            return "LT";
        case TokenType::GT:
            return "GT";
        case TokenType::LE:
            return "LE";
        case TokenType::GE:
            return "GE";
        case TokenType::AND:
            return "AND";
        case TokenType::OR:
            return "OR";
        case TokenType::ASSIGN:
            return "ASSIGN";
        case TokenType::COLON:
            return "COLON";
        case TokenType::SEMICOLON:
            return "SEMICOLON";
        case TokenType::COMMA:
            return "COMMA";
        case TokenType::DOT:
            return "DOT";
        case TokenType::LPAREN:
            return "LPAREN";
        case TokenType::RPAREN:
            return "RPAREN";
        case TokenType::LBRACK:
            return "LBRACK";
        case TokenType::RBRACK:
            return "RBRACK";
        case TokenType::LBRACE:
            return "LBRACE";
        case TokenType::RBRACE:
            return "RBRACE";
        case TokenType::EOF_TOKEN:
            return "EOF";
        case TokenType::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

}  // namespace tiger
