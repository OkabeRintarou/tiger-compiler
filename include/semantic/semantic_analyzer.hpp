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
class SemanticAnalyzer : public Visitor<TypePtr> {
public:
    explicit SemanticAnalyzer(TypeContext& typeCtx);

    TypePtr analyze(ExprPtr expr);

    // Expression visitors - return the type of the expression
    TypePtr visit(VarExpr* expr) override;
    TypePtr visit(NilExpr* expr) override;
    TypePtr visit(IntExpr* expr) override;
    TypePtr visit(StringExpr* expr) override;
    TypePtr visit(CallExpr* expr) override;
    TypePtr visit(OpExpr* expr) override;
    TypePtr visit(RecordExpr* expr) override;
    TypePtr visit(ArrayExpr* expr) override;
    TypePtr visit(AssignExpr* expr) override;
    TypePtr visit(IfExpr* expr) override;
    TypePtr visit(WhileExpr* expr) override;
    TypePtr visit(ForExpr* expr) override;
    TypePtr visit(BreakExpr* expr) override;
    TypePtr visit(LetExpr* expr) override;
    TypePtr visit(SeqExpr* expr) override;

    // Declaration visitors - return void (declarations don't have types)
    TypePtr visit(TypeDecl* decl) override;
    TypePtr visit(VarDecl* decl) override;
    TypePtr visit(FunctionDecl* decl) override;

    // Type visitors (AST type nodes, not semantic types)
    TypePtr visit(tiger::NameType* type) override { return nullptr; }
    TypePtr visit(tiger::RecordType* type) override { return nullptr; }
    TypePtr visit(tiger::ArrayType* type) override { return nullptr; }

private:
    Environment env_;
    TypePtr currentReturnType_;  // Return type of current function

    // Helper methods
    void error(const std::string& msg, int line, int column);
    TypePtr checkTypeEquals(TypePtr expected, TypePtr actual, const std::string& errorMsg, int line,
                            int column);
    TypePtr checkAssignable(TypePtr varType, TypePtr exprType, const std::string& varName, int line,
                            int column);
    TypePtr translateType(tiger::Type* astType);

    // Process type declarations in two phases for mutual recursion
    void processTypeDeclarations(const std::vector<std::shared_ptr<TypeDecl>>& typeDecls);
};

}  // namespace semantic
}  // namespace tiger

#endif  // TIGER_SEMANTIC_ANALYZER_HPP
