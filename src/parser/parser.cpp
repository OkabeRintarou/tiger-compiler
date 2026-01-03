#include "parser/parser.hpp"

#include <iostream>
#include <stdexcept>

namespace tiger {

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

Token Parser::peek() const {
    if (pos_ >= tokens_.size()) return Token(TokenType::EOF_TOKEN, "", 0, 0);
    return tokens_[pos_];
}

Token Parser::advance() {
    if (!isAtEnd()) pos_++;
    return tokens_[pos_ - 1];
}

bool Parser::isAtEnd() const {
    return pos_ >= tokens_.size() || peek().type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error(message);
}

void Parser::error(const std::string& message) const {
    Token tok = peek();
    throw SyntaxError(message, tok.line, tok.column);
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (tokens_[pos_ - 1].type == TokenType::SEMICOLON) return;

        switch (peek().type) {
            case TokenType::ARRAY:
            case TokenType::BREAK:
            case TokenType::DO:
            case TokenType::ELSE:
            case TokenType::END:
            case TokenType::FOR:
            case TokenType::FUNCTION:
            case TokenType::IF:
            case TokenType::IN:
            case TokenType::LET:
            case TokenType::TYPE:
            case TokenType::VAR:
            case TokenType::WHILE:
                return;
            default:
                break;
        }
        advance();
    }
}

ExprPtr Parser::parse() {
    try {
        return parseProgram();
    } catch (const SyntaxError& e) {
        throw;
    }
}

ExprPtr Parser::parseProgram() { return parseExpr(); }

ExprPtr Parser::parseExpr() { return parseExprOr(); }

ExprPtr Parser::parseExprOr() {
    ExprPtr left = parseExprAnd();
    while (match(TokenType::OR)) {
        ExprPtr right = parseExprAnd();
        left = std::make_shared<OpExpr>(OpExpr::Op::OR, left, right);
    }
    return left;
}

ExprPtr Parser::parseExprAnd() {
    ExprPtr left = parseExprComparison();
    while (match(TokenType::AND)) {
        ExprPtr right = parseExprComparison();
        left = std::make_shared<OpExpr>(OpExpr::Op::AND, left, right);
    }
    return left;
}

ExprPtr Parser::parseExprComparison() {
    ExprPtr left = parseExprAdditive();
    while (check(TokenType::EQ) || check(TokenType::NEQ) || check(TokenType::LT) ||
           check(TokenType::GT) || check(TokenType::LE) || check(TokenType::GE)) {
        Token op = advance();
        ExprPtr right = parseExprAdditive();
        OpExpr::Op oper = tokenToOp(op.type);
        left = std::make_shared<OpExpr>(oper, left, right);
    }
    return left;
}

ExprPtr Parser::parseExprAdditive() {
    ExprPtr left = parseExprMultiplicative();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = tokens_[pos_ - 1];
        ExprPtr right = parseExprMultiplicative();
        OpExpr::Op oper = tokenToOp(op.type);
        left = std::make_shared<OpExpr>(oper, left, right);
    }
    return left;
}

ExprPtr Parser::parseExprMultiplicative() {
    ExprPtr left = parseExprUnary();
    while (match(TokenType::TIMES) || match(TokenType::DIVIDE)) {
        Token op = tokens_[pos_ - 1];
        ExprPtr right = parseExprUnary();
        OpExpr::Op oper = tokenToOp(op.type);
        left = std::make_shared<OpExpr>(oper, left, right);
    }
    return left;
}

ExprPtr Parser::parseExprUnary() {
    if (match(TokenType::MINUS)) {
        ExprPtr expr = parseExprUnary();
        auto zero = std::make_shared<IntExpr>(0);
        return std::make_shared<OpExpr>(OpExpr::Op::MINUS, zero, expr);
    }
    return parseExprPrimary();
}

ExprPtr Parser::parseExprPrimary() {
    if (match(TokenType::NIL)) return std::make_shared<NilExpr>();

    if (check(TokenType::INTEGER)) {
        Token tok = advance();
        return std::make_shared<IntExpr>(tok.integer_value);
    }

    if (check(TokenType::STRING)) {
        Token tok = advance();
        return std::make_shared<StringExpr>(tok.lexeme);
    }

    if (check(TokenType::ID)) {
        Token id_token = advance();
        std::string id = id_token.lexeme;

        if (check(TokenType::LPAREN)) return parseCallExpr(id);
        if (check(TokenType::LBRACE)) return parseRecordExpr(id);

        if (check(TokenType::LBRACK)) {
            size_t saved_pos = pos_;
            advance();
            ExprPtr size = parseExpr();
            if (check(TokenType::RBRACK) && match(TokenType::RBRACK)) {
                if (check(TokenType::OF)) {
                    consume(TokenType::OF, "Expected 'of' in array creation");
                    ExprPtr init = parseExpr();
                    return std::make_shared<ArrayExpr>(id, size, init);
                }
            }
            pos_ = saved_pos;
            return parseLvalue(id);
        }

        return parseLvalue(id);
    }

    if (check(TokenType::LPAREN)) return parseSeqExpr();
    if (check(TokenType::IF)) return parseIfExpr();
    if (check(TokenType::WHILE)) return parseWhileExpr();
    if (check(TokenType::FOR)) return parseForExpr();
    if (check(TokenType::BREAK)) {
        advance();
        return std::make_shared<BreakExpr>();
    }
    if (check(TokenType::LET)) return parseLetExpr();

    error("Expected expression");
}

ExprPtr Parser::parseLvalue(const std::string& id) {
    auto var_expr = std::make_shared<VarExpr>(id);

    while (true) {
        if (match(TokenType::DOT)) {
            Token field = consume(TokenType::ID, "Expected field name after '.'");
            auto new_var = std::make_shared<VarExpr>(field.lexeme);
            new_var->var_kind = VarExpr::VarKind::FIELD;
            new_var->var = var_expr;
            var_expr = new_var;
        } else if (match(TokenType::LBRACK)) {
            ExprPtr index = parseExpr();
            consume(TokenType::RBRACK, "Expected ']' after array index");
            auto new_var = std::make_shared<VarExpr>("");
            new_var->var_kind = VarExpr::VarKind::SUBSCRIPT;
            new_var->var = var_expr;
            new_var->index = index;
            var_expr = new_var;
        } else {
            break;
        }
    }

    if (match(TokenType::ASSIGN)) {
        ExprPtr expr = parseExpr();
        return std::make_shared<AssignExpr>(var_expr, expr);
    }

    return var_expr;
}

ExprPtr Parser::parseCallExpr(const std::string& id) {
    consume(TokenType::LPAREN, "Expected '(' after function name");
    ExprList args;
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parseExpr());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after function arguments");
    return std::make_shared<CallExpr>(id, args);
}

ExprPtr Parser::parseRecordExpr(const std::string& type_id) {
    consume(TokenType::LBRACE, "Expected '{' for record creation");
    std::vector<std::pair<std::string, ExprPtr>> fields;

    if (!check(TokenType::RBRACE)) {
        do {
            Token id = consume(TokenType::ID, "Expected field name");
            consume(TokenType::EQ, "Expected '=' after field name");
            ExprPtr expr = parseExpr();
            fields.push_back({id.lexeme, expr});
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RBRACE, "Expected '}' after record fields");
    return std::make_shared<RecordExpr>(type_id, fields);
}

ExprPtr Parser::parseArrayExpr(const std::string& type_id) {
    consume(TokenType::LBRACK, "Expected '[' for array creation");
    ExprPtr size = parseExpr();
    consume(TokenType::RBRACK, "Expected ']' after array size");
    consume(TokenType::OF, "Expected 'of' in array creation");
    ExprPtr init = parseExpr();
    return std::make_shared<ArrayExpr>(type_id, size, init);
}

ExprPtr Parser::parseIfExpr() {
    consume(TokenType::IF, "Expected 'if'");
    ExprPtr test = parseExpr();
    consume(TokenType::THEN, "Expected 'then' after if condition");
    ExprPtr then_clause = parseExpr();
    ExprPtr else_clause = nullptr;
    if (match(TokenType::ELSE)) {
        else_clause = parseExpr();
    }
    return std::make_shared<IfExpr>(test, then_clause, else_clause);
}

ExprPtr Parser::parseWhileExpr() {
    consume(TokenType::WHILE, "Expected 'while'");
    ExprPtr test = parseExpr();
    consume(TokenType::DO, "Expected 'do' after while condition");
    ExprPtr body = parseExpr();
    return std::make_shared<WhileExpr>(test, body);
}

ExprPtr Parser::parseForExpr() {
    consume(TokenType::FOR, "Expected 'for'");
    Token id = consume(TokenType::ID, "Expected variable name after 'for'");
    consume(TokenType::ASSIGN, "Expected ':=' after for variable");
    ExprPtr lo = parseExpr();
    consume(TokenType::TO, "Expected 'to' in for loop");
    ExprPtr hi = parseExpr();
    consume(TokenType::DO, "Expected 'do' after for range");
    ExprPtr body = parseExpr();
    return std::make_shared<ForExpr>(id.lexeme, lo, hi, body);
}

ExprPtr Parser::parseLetExpr() {
    consume(TokenType::LET, "Expected 'let'");
    DeclList decls = parseDeclarationList();
    consume(TokenType::IN, "Expected 'in' after let declarations");

    ExprList body;
    if (!check(TokenType::END)) {
        do {
            body.push_back(parseExpr());
        } while (match(TokenType::SEMICOLON));
    }

    consume(TokenType::END, "Expected 'end' to terminate let");
    return std::make_shared<LetExpr>(decls, body);
}

ExprPtr Parser::parseSeqExpr() {
    consume(TokenType::LPAREN, "Expected '('");
    ExprList exprs;
    if (!check(TokenType::RPAREN)) {
        do {
            exprs.push_back(parseExpr());
        } while (match(TokenType::SEMICOLON));
    }
    consume(TokenType::RPAREN, "Expected ')'");
    return std::make_shared<SeqExpr>(exprs);
}

DeclPtr Parser::parseDeclaration() {
    if (check(TokenType::TYPE)) return parseTypeDeclaration();
    if (check(TokenType::VAR)) return parseVarDeclaration();
    if (check(TokenType::FUNCTION)) return parseFunctionDeclaration();
    error("Expected declaration");
}

DeclList Parser::parseDeclarationList() {
    DeclList decls;
    while (check(TokenType::TYPE) || check(TokenType::VAR) || check(TokenType::FUNCTION)) {
        decls.push_back(parseDeclaration());
    }
    return decls;
}

DeclPtr Parser::parseTypeDeclaration() {
    consume(TokenType::TYPE, "Expected 'type'");
    Token id = consume(TokenType::ID, "Expected type name");
    consume(TokenType::EQ, "Expected '=' after type name");
    TypePtr type = parseType();
    return std::make_shared<TypeDecl>(id.lexeme, type);
}

TypePtr Parser::parseType() {
    if (check(TokenType::LBRACE)) {
        consume(TokenType::LBRACE, "Expected '{' for record type");
        FieldList fields;
        if (!check(TokenType::RBRACE)) {
            do {
                fields.push_back(parseTypeField());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RBRACE, "Expected '}' after record fields");
        auto record = std::make_shared<RecordType>();
        record->fields = fields;
        return record;
    }

    if (check(TokenType::ARRAY)) {
        consume(TokenType::ARRAY, "Expected 'array'");
        consume(TokenType::OF, "Expected 'of' in array type");
        Token id = consume(TokenType::ID, "Expected element type");
        return std::make_shared<ArrayType>(id.lexeme);
    }

    Token id = consume(TokenType::ID, "Expected type name");
    return std::make_shared<NameType>(id.lexeme);
}

DeclPtr Parser::parseVarDeclaration() {
    consume(TokenType::VAR, "Expected 'var'");
    Token id = consume(TokenType::ID, "Expected variable name");

    std::string type_id = "";
    if (match(TokenType::COLON)) {
        Token type = consume(TokenType::ID, "Expected type name");
        type_id = type.lexeme;
    }

    consume(TokenType::ASSIGN, "Expected ':=' in variable declaration");
    ExprPtr init = parseExpr();

    return std::make_shared<VarDecl>(id.lexeme, type_id, init);
}

DeclPtr Parser::parseFunctionDeclaration() {
    consume(TokenType::FUNCTION, "Expected 'function'");
    Token id = consume(TokenType::ID, "Expected function name");

    consume(TokenType::LPAREN, "Expected '(' after function name");
    FieldList params;
    if (!check(TokenType::RPAREN)) {
        params = parseTypeFields();
    }
    consume(TokenType::RPAREN, "Expected ')' after function parameters");

    std::string result_type = "";
    if (match(TokenType::COLON)) {
        Token type = consume(TokenType::ID, "Expected return type");
        result_type = type.lexeme;
    }

    consume(TokenType::EQ, "Expected '=' before function body");
    ExprPtr body = parseExpr();

    return std::make_shared<FunctionDecl>(id.lexeme, params, result_type, body);
}

FieldList Parser::parseTypeFields() {
    FieldList fields;
    do {
        fields.push_back(parseTypeField());
    } while (match(TokenType::COMMA));
    return fields;
}

FieldPtr Parser::parseTypeField() {
    Token name = consume(TokenType::ID, "Expected parameter name");
    consume(TokenType::COLON, "Expected ':' after parameter name");
    Token type = consume(TokenType::ID, "Expected parameter type");
    return std::make_shared<Field>(name.lexeme, type.lexeme, false);
}

bool Parser::isBinaryOp(TokenType type) const {
    switch (type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::TIMES:
        case TokenType::DIVIDE:
        case TokenType::EQ:
        case TokenType::NEQ:
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LE:
        case TokenType::GE:
        case TokenType::AND:
        case TokenType::OR:
            return true;
        default:
            return false;
    }
}

OpExpr::Op Parser::tokenToOp(TokenType type) const {
    switch (type) {
        case TokenType::PLUS:
            return OpExpr::Op::PLUS;
        case TokenType::MINUS:
            return OpExpr::Op::MINUS;
        case TokenType::TIMES:
            return OpExpr::Op::TIMES;
        case TokenType::DIVIDE:
            return OpExpr::Op::DIVIDE;
        case TokenType::EQ:
            return OpExpr::Op::EQ;
        case TokenType::NEQ:
            return OpExpr::Op::NEQ;
        case TokenType::LT:
            return OpExpr::Op::LT;
        case TokenType::GT:
            return OpExpr::Op::GT;
        case TokenType::LE:
            return OpExpr::Op::LE;
        case TokenType::GE:
            return OpExpr::Op::GE;
        case TokenType::AND:
            return OpExpr::Op::AND;
        case TokenType::OR:
            return OpExpr::Op::OR;
        default:
            error("Token is not an operator");
    }
}

}  // namespace tiger
