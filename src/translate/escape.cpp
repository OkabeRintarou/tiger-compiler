#include "translate/escape.hpp"

namespace tiger {
namespace translate {

void EscapeAnalyzer::analyze(ast::Expr* expr) {
    beginScope();
    if (expr) {
        expr->accept(*this);
    }
    endScope();
}

void EscapeAnalyzer::beginScope() { envStack_.emplace_back(); }
void EscapeAnalyzer::endScope() {
    if (!envStack_.empty()) envStack_.pop_back();
}

void EscapeAnalyzer::enterVar(const std::string& name, bool* escapeFlag) {
    if (!envStack_.empty()) {
        envStack_.back()[name] = EscapeEntry(depth_, escapeFlag);
    }
}

void EscapeAnalyzer::checkEscape(const std::string& name) {
    for (auto it = envStack_.rbegin(); it != envStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            if (depth_ > found->second.depth && found->second.escape) {
                *(found->second.escape) = true;
            }
            return;
        }
    }
}

void EscapeAnalyzer::visit(ast::VarExpr* expr) {
    switch (expr->var_kind) {
        case ast::VarExpr::VarKind::SIMPLE:
            checkEscape(expr->name);
            break;
        case ast::VarExpr::VarKind::FIELD:
            if (expr->var) expr->var->accept(*this);
            break;
        case ast::VarExpr::VarKind::SUBSCRIPT:
            if (expr->var) expr->var->accept(*this);
            if (expr->index) expr->index->accept(*this);
            break;
    }
}

void EscapeAnalyzer::visit(ast::CallExpr* expr) {
    for (auto& arg : expr->args) {
        if (arg) arg->accept(*this);
    }
}

void EscapeAnalyzer::visit(ast::OpExpr* expr) {
    if (expr->left) expr->left->accept(*this);
    if (expr->right) expr->right->accept(*this);
}

void EscapeAnalyzer::visit(ast::RecordExpr* expr) {
    for (auto& field : expr->fields) {
        if (field.second) field.second->accept(*this);
    }
}

void EscapeAnalyzer::visit(ast::ArrayExpr* expr) {
    if (expr->size) expr->size->accept(*this);
    if (expr->init) expr->init->accept(*this);
}

void EscapeAnalyzer::visit(ast::AssignExpr* expr) {
    if (expr->var) expr->var->accept(*this);
    if (expr->expr) expr->expr->accept(*this);
}

void EscapeAnalyzer::visit(ast::IfExpr* expr) {
    if (expr->test) expr->test->accept(*this);
    if (expr->then_clause) expr->then_clause->accept(*this);
    if (expr->else_clause) expr->else_clause->accept(*this);
}

void EscapeAnalyzer::visit(ast::WhileExpr* expr) {
    if (expr->test) expr->test->accept(*this);
    if (expr->body) expr->body->accept(*this);
}

void EscapeAnalyzer::visit(ast::ForExpr* expr) {
    beginScope();
    enterVar(expr->var, &expr->escape);
    if (expr->lo) expr->lo->accept(*this);
    if (expr->hi) expr->hi->accept(*this);
    if (expr->body) expr->body->accept(*this);
    endScope();
}

void EscapeAnalyzer::visit(ast::LetExpr* expr) {
    beginScope();
    for (auto& decl : expr->decls) {
        if (decl) decl->accept(*this);
    }
    for (auto& e : expr->body) {
        if (e) e->accept(*this);
    }
    endScope();
}

void EscapeAnalyzer::visit(ast::SeqExpr* expr) {
    for (auto& e : expr->exprs) {
        if (e) e->accept(*this);
    }
}

void EscapeAnalyzer::visit(ast::VarDecl* decl) {
    if (decl->init) decl->init->accept(*this);
    enterVar(decl->name, &decl->escape);
}

void EscapeAnalyzer::visit(ast::FunctionDecl* decl) {
    depth_++;
    beginScope();
    for (auto& param : decl->params) {
        if (param) enterVar(param->name, &param->escape);
    }
    if (decl->body) decl->body->accept(*this);
    endScope();
    depth_--;
}

}  // namespace translate
}  // namespace tiger
