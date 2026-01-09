#ifndef TIGER_TRANSLATE_ESCAPE_HPP
#define TIGER_TRANSLATE_ESCAPE_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "ast/ast.hpp"

namespace tiger {
namespace translate {

/**
 * EscapeEntry - Information about a variable's escape status
 */
struct EscapeEntry {
    int depth;
    bool* escape;
    EscapeEntry() : depth(0), escape(nullptr) {}
    EscapeEntry(int d, bool* e) : depth(d), escape(e) {}
};

/**
 * EscapeAnalyzer - Determines which variables escape
 *
 * A variable "escapes" if accessed from a nested function.
 * Such variables must be in stack frame, not registers.
 */
class EscapeAnalyzer : public ast::Visitor<void> {
public:
    EscapeAnalyzer() : depth_(0) {}

    void analyze(ast::Expr* expr);

    // Expression visitors
    void visit(ast::VarExpr* expr) override;
    void visit(ast::NilExpr*) override {}
    void visit(ast::IntExpr*) override {}
    void visit(ast::StringExpr*) override {}
    void visit(ast::CallExpr* expr) override;
    void visit(ast::OpExpr* expr) override;
    void visit(ast::RecordExpr* expr) override;
    void visit(ast::ArrayExpr* expr) override;
    void visit(ast::AssignExpr* expr) override;
    void visit(ast::IfExpr* expr) override;
    void visit(ast::WhileExpr* expr) override;
    void visit(ast::ForExpr* expr) override;
    void visit(ast::BreakExpr*) override {}
    void visit(ast::LetExpr* expr) override;
    void visit(ast::SeqExpr* expr) override;

    // Declaration visitors
    void visit(ast::TypeDecl*) override {}
    void visit(ast::VarDecl* decl) override;
    void visit(ast::FunctionDecl* decl) override;

    // Type visitors
    void visit(ast::NameType*) override {}
    void visit(ast::RecordType*) override {}
    void visit(ast::ArrayType*) override {}

private:
    using EscapeEnv = std::unordered_map<std::string, EscapeEntry>;
    std::vector<EscapeEnv> envStack_;
    int depth_;

    void beginScope();
    void endScope();
    void enterVar(const std::string& name, bool* escapeFlag);
    void checkEscape(const std::string& name);
};

inline void findEscapes(ast::ExprPtr& expr) {
    EscapeAnalyzer analyzer;
    analyzer.analyze(expr.get());
}

}  // namespace translate
}  // namespace tiger

#endif  // TIGER_TRANSLATE_ESCAPE_HPP
