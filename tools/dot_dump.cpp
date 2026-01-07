#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

using namespace tiger;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

class AstDumper : public Visitor<void> {
    std::ostream& out;

    std::string getNodeId(const void* node) {
        std::stringstream ss;
        ss << "node" << (size_t)node;
        return ss.str();
    }

    void emitNode(const void* node, const std::string& label) {
        out << "    " << getNodeId(node) << " [label=\"" << label << "\"];" << std::endl;
    }

    void emitEdge(const void* from, const void* to, const std::string& label = "") {
        if (!to) return;
        out << "    " << getNodeId(from) << " -> " << getNodeId(to);
        if (!label.empty()) {
            out << " [label=\"" << label << "\"]";
        }
        out << ";" << std::endl;
    }

public:
    explicit AstDumper(std::ostream& os) : out(os) {}

    void dump(ExprPtr root) {
        out << "digraph AST {" << std::endl;
        out << "    node [shape=box];" << std::endl;
        root->accept(*this);
        out << "}" << std::endl;
    }

    void visit(VarExpr* expr) override {
        std::string label = "VarExpr";
        if (expr->var_kind == VarExpr::VarKind::SIMPLE) {
            label += ": " + expr->name;
            emitNode(expr, label);
        } else if (expr->var_kind == VarExpr::VarKind::FIELD) {
            label += "(Field): " + expr->name;
            emitNode(expr, label);
            if (expr->var) {
                expr->var->accept(*this);
                emitEdge(expr, expr->var.get(), "record");
            }
        } else if (expr->var_kind == VarExpr::VarKind::SUBSCRIPT) {
            label += "(Subscript)";
            emitNode(expr, label);
            if (expr->var) {
                expr->var->accept(*this);
                emitEdge(expr, expr->var.get(), "array");
            }
            if (expr->index) {
                expr->index->accept(*this);
                emitEdge(expr, expr->index.get(), "index");
            }
        }
    }

    void visit(NilExpr* expr) override { emitNode(expr, "Nil"); }

    void visit(IntExpr* expr) override { emitNode(expr, "Int: " + std::to_string(expr->value)); }

    void visit(StringExpr* expr) override { emitNode(expr, "String: " + expr->value); }

    void visit(CallExpr* expr) override {
        emitNode(expr, "Call: " + expr->func);
        for (const auto& arg : expr->args) {
            arg->accept(*this);
            emitEdge(expr, arg.get(), "arg");
        }
    }

    void visit(OpExpr* expr) override {
        emitNode(expr, "Op: " + opToString(expr->oper));
        if (expr->left) {
            expr->left->accept(*this);
            emitEdge(expr, expr->left.get(), "L");
        }
        if (expr->right) {
            expr->right->accept(*this);
            emitEdge(expr, expr->right.get(), "R");
        }
    }

    void visit(RecordExpr* expr) override {
        emitNode(expr, "Record: " + expr->type_id);
        for (const auto& field : expr->fields) {
            field.second->accept(*this);
            emitEdge(expr, field.second.get(), field.first);
        }
    }

    void visit(ArrayExpr* expr) override {
        emitNode(expr, "Array: " + expr->type_id);
        if (expr->size) {
            expr->size->accept(*this);
            emitEdge(expr, expr->size.get(), "size");
        }
        if (expr->init) {
            expr->init->accept(*this);
            emitEdge(expr, expr->init.get(), "init");
        }
    }

    void visit(AssignExpr* expr) override {
        emitNode(expr, "Assign");
        if (expr->var) {
            expr->var->accept(*this);
            emitEdge(expr, expr->var.get(), "var");
        }
        if (expr->expr) {
            expr->expr->accept(*this);
            emitEdge(expr, expr->expr.get(), "expr");
        }
    }

    void visit(IfExpr* expr) override {
        emitNode(expr, "If");
        if (expr->test) {
            expr->test->accept(*this);
            emitEdge(expr, expr->test.get(), "test");
        }
        if (expr->then_clause) {
            expr->then_clause->accept(*this);
            emitEdge(expr, expr->then_clause.get(), "then");
        }
        if (expr->else_clause) {
            expr->else_clause->accept(*this);
            emitEdge(expr, expr->else_clause.get(), "else");
        }
    }

    void visit(WhileExpr* expr) override {
        emitNode(expr, "While");
        if (expr->test) {
            expr->test->accept(*this);
            emitEdge(expr, expr->test.get(), "test");
        }
        if (expr->body) {
            expr->body->accept(*this);
            emitEdge(expr, expr->body.get(), "body");
        }
    }

    void visit(ForExpr* expr) override {
        emitNode(expr, "For: " + expr->var);
        if (expr->lo) {
            expr->lo->accept(*this);
            emitEdge(expr, expr->lo.get(), "lo");
        }
        if (expr->hi) {
            expr->hi->accept(*this);
            emitEdge(expr, expr->hi.get(), "hi");
        }
        if (expr->body) {
            expr->body->accept(*this);
            emitEdge(expr, expr->body.get(), "body");
        }
    }

    void visit(BreakExpr* expr) override { emitNode(expr, "Break"); }

    void visit(LetExpr* expr) override {
        emitNode(expr, "Let");
        for (const auto& decl : expr->decls) {
            decl->accept(*this);
            emitEdge(expr, decl.get(), "decl");
        }
        for (const auto& e : expr->body) {
            e->accept(*this);
            emitEdge(expr, e.get(), "body");
        }
    }

    void visit(SeqExpr* expr) override {
        emitNode(expr, "Seq");
        for (const auto& e : expr->exprs) {
            e->accept(*this);
            emitEdge(expr, e.get());
        }
    }

    void visit(TypeDecl* decl) override {
        emitNode(decl, "TypeDecl: " + decl->name);
        if (decl->type) {
            decl->type->accept(*this);
            emitEdge(decl, decl->type.get(), "type");
        }
    }

    void visit(VarDecl* decl) override {
        emitNode(decl, "VarDecl: " + decl->name);
        if (decl->init) {
            decl->init->accept(*this);
            emitEdge(decl, decl->init.get(), "init");
        }
    }

    void visit(FunctionDecl* decl) override {
        emitNode(decl, "FunctionDecl: " + decl->name);
        if (decl->body) {
            decl->body->accept(*this);
            emitEdge(decl, decl->body.get(), "body");
        }
    }

    void visit(NameType* type) override { emitNode(type, "NameType: " + type->name); }

    void visit(RecordType* type) override { emitNode(type, "RecordType"); }

    void visit(ArrayType* type) override { emitNode(type, "ArrayType: " + type->element_type); }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);

        tiger::Lexer lexer(source);
        std::vector<tiger::Token> tokens = lexer.tokenize();

        tiger::Parser parser(tokens);
        tiger::ExprPtr ast = parser.parse();

        AstDumper dumper(std::cout);
        dumper.dump(ast);

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
