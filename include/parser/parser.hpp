#ifndef TIGER_PARSER_HPP
#define TIGER_PARSER_HPP

#include <memory>
#include <vector>

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

namespace tiger {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    ExprPtr parse();

private:
    const std::vector<Token>& tokens_;
    size_t pos_;

    // Token navigation
    Token peek() const;
    Token advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);

    // Error handling
    [[noreturn]] void error(const std::string& message) const;
    void synchronize();

    // Parsing functions
    ExprPtr parseProgram();
    ExprPtr parseExpr();

    // Expression parsing (precedence levels)
    ExprPtr parseExprOr();
    ExprPtr parseExprAnd();
    ExprPtr parseExprComparison();
    ExprPtr parseExprAdditive();
    ExprPtr parseExprMultiplicative();
    ExprPtr parseExprUnary();
    ExprPtr parseExprPrimary();

    // Complex expressions
    ExprPtr parseLvalue(const std::string& id);
    ExprPtr parseCallExpr(const std::string& id);
    ExprPtr parseRecordExpr(const std::string& type_id);
    ExprPtr parseArrayExpr(const std::string& type_id);
    ExprPtr parseIfExpr();
    ExprPtr parseWhileExpr();
    ExprPtr parseForExpr();
    ExprPtr parseLetExpr();
    ExprPtr parseSeqExpr();

    // Declaration parsing
    DeclPtr parseDeclaration();
    DeclList parseDeclarationList();
    DeclPtr parseTypeDeclaration();
    TypePtr parseType();
    DeclPtr parseVarDeclaration();
    DeclPtr parseFunctionDeclaration();
    FieldList parseTypeFields();
    FieldPtr parseTypeField();

    // Utility
    bool isBinaryOp(TokenType type) const;
    OpExpr::Op tokenToOp(TokenType type) const;
};

}  // namespace tiger

#endif  // TIGER_PARSER_HPP
