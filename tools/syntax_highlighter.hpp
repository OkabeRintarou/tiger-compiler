// filepath: /home/jenkins/code/tiger-compiler/tools/syntax_highlighter.hpp
#ifndef TIGER_SYNTAX_HIGHLIGHTER_HPP
#define TIGER_SYNTAX_HIGHLIGHTER_HPP

#include <sstream>
#include <string>

#include "ast/ast.hpp"

namespace tiger {
namespace tools {

/**
 * @brief Syntax highlighter using visitor pattern for terminal output
 *
 * This visitor traverses the AST and generates terminal output with ANSI color codes.
 * Different language constructs are colored differently for better code readability.
 */
class SyntaxHighlighter : public Visitor<std::string> {
public:
    SyntaxHighlighter();

    /**
     * @brief Generate highlighted output from an expression
     * @param expr The root expression to highlight
     * @return String with ANSI color codes for terminal display
     */
    std::string highlight(ExprPtr expr);

    // Expression visitors
    std::string visit(VarExpr* expr) override;
    std::string visit(NilExpr* expr) override;
    std::string visit(IntExpr* expr) override;
    std::string visit(StringExpr* expr) override;
    std::string visit(CallExpr* expr) override;
    std::string visit(OpExpr* expr) override;
    std::string visit(RecordExpr* expr) override;
    std::string visit(ArrayExpr* expr) override;
    std::string visit(AssignExpr* expr) override;
    std::string visit(IfExpr* expr) override;
    std::string visit(WhileExpr* expr) override;
    std::string visit(ForExpr* expr) override;
    std::string visit(BreakExpr* expr) override;
    std::string visit(LetExpr* expr) override;
    std::string visit(SeqExpr* expr) override;

    // Declaration visitors
    std::string visit(TypeDecl* decl) override;
    std::string visit(VarDecl* decl) override;
    std::string visit(FunctionDecl* decl) override;

    // Type visitors
    std::string visit(NameType* type) override;
    std::string visit(RecordType* type) override;
    std::string visit(ArrayType* type) override;

private:
    int indent_level_;

    // ANSI color codes
    static const std::string RESET;
    static const std::string BOLD;
    static const std::string KEYWORD_COLOR;     // Magenta + Bold
    static const std::string TYPE_COLOR;        // Yellow + Bold
    static const std::string FUNCTION_COLOR;    // Blue + Bold
    static const std::string IDENTIFIER_COLOR;  // Cyan
    static const std::string STRING_COLOR;      // Green
    static const std::string NUMBER_COLOR;      // Yellow
    static const std::string OPERATOR_COLOR;    // White + Bold
    static const std::string COMMENT_COLOR;     // Gray

    // Helper methods for formatting
    std::string indent() const;
    void increaseIndent();
    void decreaseIndent();

    // ANSI color wrappers for different syntax elements
    std::string keyword(const std::string& text) const;
    std::string identifier(const std::string& text) const;
    std::string type_name(const std::string& text) const;
    std::string literal(const std::string& text) const;
    std::string string_literal(const std::string& text) const;
    std::string operator_text(const std::string& text) const;
    std::string comment(const std::string& text) const;
    std::string function_name(const std::string& text) const;
};

}  // namespace tools
}  // namespace tiger

#endif  // TIGER_SYNTAX_HIGHLIGHTER_HPP
