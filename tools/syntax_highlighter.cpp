#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

namespace tiger {
namespace tools {
using namespace tiger::ast;

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

// ANSI color code definitions
const std::string SyntaxHighlighter::RESET = "\033[0m";
const std::string SyntaxHighlighter::BOLD = "\033[1m";
const std::string SyntaxHighlighter::KEYWORD_COLOR = "\033[1;35m";   // Bold Magenta
const std::string SyntaxHighlighter::TYPE_COLOR = "\033[1;33m";      // Bold Yellow
const std::string SyntaxHighlighter::FUNCTION_COLOR = "\033[1;34m";  // Bold Blue
const std::string SyntaxHighlighter::IDENTIFIER_COLOR = "\033[36m";  // Cyan
const std::string SyntaxHighlighter::STRING_COLOR = "\033[32m";      // Green
const std::string SyntaxHighlighter::NUMBER_COLOR = "\033[33m";      // Yellow
const std::string SyntaxHighlighter::OPERATOR_COLOR = "\033[1;37m";  // Bold White
const std::string SyntaxHighlighter::COMMENT_COLOR = "\033[90m";     // Gray

SyntaxHighlighter::SyntaxHighlighter() : indent_level_(0) {}

std::string SyntaxHighlighter::highlight(ExprPtr expr) {
    if (!expr) return "";
    return expr->accept(*this);
}

// ========== Expression Visitors ==========

std::string SyntaxHighlighter::visit(VarExpr* expr) {
    std::ostringstream result;
    switch (expr->var_kind) {
        case VarExpr::VarKind::SIMPLE:
            result << identifier(expr->name);
            break;
        case VarExpr::VarKind::FIELD:
            result << expr->var->accept(*this) << operator_text(".") << identifier(expr->name);
            break;
        case VarExpr::VarKind::SUBSCRIPT:
            result << expr->var->accept(*this) << operator_text("[") << expr->index->accept(*this)
                   << operator_text("]");
            break;
    }
    return result.str();
}

std::string SyntaxHighlighter::visit(NilExpr*) { return keyword("nil"); }

std::string SyntaxHighlighter::visit(IntExpr* expr) { return literal(std::to_string(expr->value)); }

std::string SyntaxHighlighter::visit(StringExpr* expr) {
    return string_literal("\"" + expr->value + "\"");
}

std::string SyntaxHighlighter::visit(CallExpr* expr) {
    std::ostringstream result;
    result << function_name(expr->func) << operator_text("(");
    for (size_t i = 0; i < expr->args.size(); ++i) {
        if (i > 0) result << operator_text(", ");
        result << expr->args[i]->accept(*this);
    }
    result << operator_text(")");
    return result.str();
}

std::string SyntaxHighlighter::visit(OpExpr* expr) {
    std::ostringstream result;
    bool needs_parens = (expr->left->kind == Expr::Kind::OP || expr->right->kind == Expr::Kind::OP);
    if (needs_parens) result << operator_text("(");
    result << expr->left->accept(*this) << " " << operator_text(opToString(expr->oper)) << " "
           << expr->right->accept(*this);
    if (needs_parens) result << operator_text(")");
    return result.str();
}

std::string SyntaxHighlighter::visit(RecordExpr* expr) {
    std::ostringstream result;
    result << type_name(expr->type_id) << operator_text(" {");
    for (size_t i = 0; i < expr->fields.size(); ++i) {
        if (i > 0) result << operator_text(", ");
        result << identifier(expr->fields[i].first) << operator_text(" = ")
               << expr->fields[i].second->accept(*this);
    }
    result << operator_text("}");
    return result.str();
}

std::string SyntaxHighlighter::visit(ArrayExpr* expr) {
    std::ostringstream result;
    result << type_name(expr->type_id) << operator_text(" [") << expr->size->accept(*this)
           << operator_text("] ") << keyword("of") << " " << expr->init->accept(*this);
    return result.str();
}

std::string SyntaxHighlighter::visit(AssignExpr* expr) {
    return expr->var->accept(*this) + " " + operator_text(":=") + " " + expr->expr->accept(*this);
}

std::string SyntaxHighlighter::visit(IfExpr* expr) {
    std::ostringstream result;
    result << keyword("if") << " " << expr->test->accept(*this) << "\n"
           << indent() << keyword("then") << " ";
    increaseIndent();
    result << expr->then_clause->accept(*this);
    decreaseIndent();
    if (expr->else_clause) {
        result << "\n" << indent() << keyword("else") << " ";
        increaseIndent();
        result << expr->else_clause->accept(*this);
        decreaseIndent();
    }
    return result.str();
}

std::string SyntaxHighlighter::visit(WhileExpr* expr) {
    std::ostringstream result;
    result << keyword("while") << " " << expr->test->accept(*this) << " " << keyword("do") << "\n";
    increaseIndent();
    result << indent() << expr->body->accept(*this);
    decreaseIndent();
    return result.str();
}

std::string SyntaxHighlighter::visit(ForExpr* expr) {
    std::ostringstream result;
    result << keyword("for") << " " << identifier(expr->var) << " " << operator_text(":=") << " "
           << expr->lo->accept(*this) << " " << keyword("to") << " " << expr->hi->accept(*this)
           << " " << keyword("do") << "\n";
    increaseIndent();
    result << indent() << expr->body->accept(*this);
    decreaseIndent();
    return result.str();
}

std::string SyntaxHighlighter::visit(BreakExpr*) { return keyword("break"); }

std::string SyntaxHighlighter::visit(LetExpr* expr) {
    std::ostringstream result;
    result << keyword("let") << "\n";
    increaseIndent();
    for (const auto& decl : expr->decls) {
        result << indent() << decl->accept(*this) << "\n";
    }
    decreaseIndent();
    result << keyword("in") << "\n";
    increaseIndent();
    for (size_t i = 0; i < expr->body.size(); ++i) {
        if (i > 0) result << operator_text(";") << "\n";
        result << indent() << expr->body[i]->accept(*this);
    }
    decreaseIndent();
    result << "\n" << keyword("end");
    return result.str();
}

std::string SyntaxHighlighter::visit(SeqExpr* expr) {
    std::ostringstream result;
    result << operator_text("(");
    for (size_t i = 0; i < expr->exprs.size(); ++i) {
        if (i > 0) result << operator_text("; ");
        result << expr->exprs[i]->accept(*this);
    }
    result << operator_text(")");
    return result.str();
}

// ========== Declaration Visitors ==========

std::string SyntaxHighlighter::visit(TypeDecl* decl) {
    return keyword("type") + " " + type_name(decl->name) + " " + operator_text("=") + " " +
           decl->type->accept(*this);
}

std::string SyntaxHighlighter::visit(VarDecl* decl) {
    std::ostringstream result;
    result << keyword("var") << " " << identifier(decl->name);
    if (!decl->type_id.empty()) {
        result << operator_text(":") << type_name(decl->type_id);
    }
    result << " " << operator_text(":=") << " " << decl->init->accept(*this);
    return result.str();
}

std::string SyntaxHighlighter::visit(FunctionDecl* decl) {
    std::ostringstream result;
    result << keyword("function") << " " << function_name(decl->name) << operator_text("(");
    for (size_t i = 0; i < decl->params.size(); ++i) {
        if (i > 0) result << operator_text(", ");
        result << identifier(decl->params[i]->name) << operator_text(":")
               << type_name(decl->params[i]->type_id);
    }
    result << operator_text(")");
    if (!decl->result_type.empty()) {
        result << operator_text(":") << type_name(decl->result_type);
    }
    result << " " << operator_text("=") << "\n";
    increaseIndent();
    result << indent() << decl->body->accept(*this);
    decreaseIndent();
    return result.str();
}

// ========== Type Visitors ==========

std::string SyntaxHighlighter::visit(NameType* type) { return type_name(type->name); }

std::string SyntaxHighlighter::visit(RecordType* type) {
    std::ostringstream result;
    result << operator_text("{");
    for (size_t i = 0; i < type->fields.size(); ++i) {
        if (i > 0) result << operator_text(", ");
        result << identifier(type->fields[i]->name) << operator_text(":")
               << type_name(type->fields[i]->type_id);
    }
    result << operator_text("}");
    return result.str();
}

std::string SyntaxHighlighter::visit(ArrayType* type) {
    return keyword("array") + " " + keyword("of") + " " + type_name(type->element_type);
}

// ========== Helper Methods ==========

std::string SyntaxHighlighter::indent() const { return std::string(indent_level_ * 2, ' '); }

void SyntaxHighlighter::increaseIndent() { indent_level_++; }

void SyntaxHighlighter::decreaseIndent() {
    if (indent_level_ > 0) indent_level_--;
}

// ========== ANSI Color Wrappers ==========

std::string SyntaxHighlighter::keyword(const std::string& text) const {
    return KEYWORD_COLOR + text + RESET;
}

std::string SyntaxHighlighter::identifier(const std::string& text) const {
    return IDENTIFIER_COLOR + text + RESET;
}

std::string SyntaxHighlighter::type_name(const std::string& text) const {
    return TYPE_COLOR + text + RESET;
}

std::string SyntaxHighlighter::literal(const std::string& text) const {
    return NUMBER_COLOR + text + RESET;
}

std::string SyntaxHighlighter::string_literal(const std::string& text) const {
    return STRING_COLOR + text + RESET;
}

std::string SyntaxHighlighter::operator_text(const std::string& text) const {
    return OPERATOR_COLOR + text + RESET;
}

std::string SyntaxHighlighter::comment(const std::string& text) const {
    return COMMENT_COLOR + text + RESET;
}

std::string SyntaxHighlighter::function_name(const std::string& text) const {
    return FUNCTION_COLOR + text + RESET;
}

}  // namespace tools
}  // namespace tiger

// ========== Helper Function ==========

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ========== Main Function ==========

int main(int argc, char* argv[]) {
    using namespace tiger;
    using namespace tiger::tools;

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input.tig>" << std::endl;
        std::cout << "\nThis tool displays syntax-highlighted Tiger source code in the terminal."
                  << std::endl;
        return 1;
    }

    std::string input_file = argv[1];

    try {
        std::string source = readFile(input_file);

        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        ExprPtr ast = parser.parse();

        SyntaxHighlighter highlighter;
        std::string highlighted = highlighter.highlight(ast);

        std::cout << "\n" << highlighted << "\n" << std::endl;

        return 0;

    } catch (const LexicalError& e) {
        std::cerr << "\033[1;31mLexical error\033[0m at line " << e.line() << ", column "
                  << e.column() << ": " << e.what() << std::endl;
        return 1;
    } catch (const SyntaxError& e) {
        std::cerr << "\033[1;31mSyntax error\033[0m at line " << e.line() << ", column "
                  << e.column() << ": " << e.what() << std::endl;
        return 1;
    } catch (const TigerError& e) {
        std::cerr << "\033[1;31mError:\033[0m " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\033[1;31mError:\033[0m " << e.what() << std::endl;
        return 1;
    }
}
