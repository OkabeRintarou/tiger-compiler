#ifndef TIGER_SEMANTIC_ANALYZER_HPP
#define TIGER_SEMANTIC_ANALYZER_HPP

#include <stdexcept>

#include "ast/ast.hpp"
#include "semantic/environment.hpp"
#include "semantic/types.hpp"

namespace tiger {
namespace semantic {

// Semantic error class
class SemanticError : public std::runtime_error {
public:
    SemanticError(const std::string& msg, int line, int column)
        : std::runtime_error(msg), line_(line), column_(column) {}

    int line() const { return line_; }
    int column() const { return column_; }

private:
    int line_;
    int column_;
};

// Semantic analyzer visitor
// Performs type checking and symbol table management
class SemanticAnalyzer : public ast::Visitor<TypePtr> {
public:
    explicit SemanticAnalyzer(TypeContext& typeCtx);

    TypePtr analyze(ast::ExprPtr expr);

    // Expression visitors - return the type of the expression
    TypePtr visit(ast::VarExpr* expr) override;
    TypePtr visit(ast::NilExpr* expr) override;
    TypePtr visit(ast::IntExpr* expr) override;
    TypePtr visit(ast::StringExpr* expr) override;
    TypePtr visit(ast::CallExpr* expr) override;
    TypePtr visit(ast::OpExpr* expr) override;
    TypePtr visit(ast::RecordExpr* expr) override;
    TypePtr visit(ast::ArrayExpr* expr) override;
    TypePtr visit(ast::AssignExpr* expr) override;
    TypePtr visit(ast::IfExpr* expr) override;
    TypePtr visit(ast::WhileExpr* expr) override;
    TypePtr visit(ast::ForExpr* expr) override;
    TypePtr visit(ast::BreakExpr* expr) override;
    TypePtr visit(ast::LetExpr* expr) override;
    TypePtr visit(ast::SeqExpr* expr) override;

    // Declaration visitors - return void (declarations don't have types)
    TypePtr visit(ast::TypeDecl* decl) override;
    TypePtr visit(ast::VarDecl* decl) override;
    TypePtr visit(ast::FunctionDecl* decl) override;

    // Type visitors (AST type nodes, not semantic types)
    TypePtr visit(ast::NameType*) override { return nullptr; }
    TypePtr visit(ast::RecordType*) override { return nullptr; }
    TypePtr visit(ast::ArrayType*) override { return nullptr; }

private:
    Environment env_;
    TypePtr currentReturnType_;  // Return type of current function

    // Helper methods
    void error(const std::string& msg, int line, int column);
    TypePtr checkTypeEquals(TypePtr expected, TypePtr actual, const std::string& errorMsg, int line,
                            int column);
    TypePtr checkAssignable(TypePtr varType, TypePtr exprType, const std::string& varName, int line,
                            int column);
    TypePtr translateType(ast::Type* astType);

    // Process type declarations in two phases for mutual recursion
    void processTypeDeclarations(const std::vector<ast::TypeDeclPtr>& typeDecls);

    void processFunctionDeclarations(const std::vector<ast::FunctionDeclPtr>& funcDecls);
};

}  // namespace semantic
}  // namespace tiger

#endif  // TIGER_SEMANTIC_ANALYZER_HPP
