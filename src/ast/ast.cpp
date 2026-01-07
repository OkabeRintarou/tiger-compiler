#include "ast/ast.hpp"

#include <sstream>

namespace tiger {

// ========== Utility Functions ==========

std::string opToString(OpExpr::Op op) {
    switch (op) {
        case OpExpr::Op::PLUS:
            return "+";
        case OpExpr::Op::MINUS:
            return "-";
        case OpExpr::Op::TIMES:
            return "*";
        case OpExpr::Op::DIVIDE:
            return "/";
        case OpExpr::Op::EQ:
            return "=";
        case OpExpr::Op::NEQ:
            return "<>";
        case OpExpr::Op::LT:
            return "<";
        case OpExpr::Op::GT:
            return ">";
        case OpExpr::Op::LE:
            return "<=";
        case OpExpr::Op::GE:
            return ">=";
        case OpExpr::Op::AND:
            return "&";
        case OpExpr::Op::OR:
            return "|";
        default:
            return "?";
    }
}

// ========== AST toString Methods ==========

std::string VarExpr::toString() const {
    std::ostringstream oss;
    if (var_kind == VarKind::SIMPLE) {
        oss << "Var(" << name << ")";
    } else if (var_kind == VarKind::FIELD) {
        oss << "FieldVar(" << var->toString() << "." << name << ")";
    } else {  // SUBSCRIPT
        oss << "SubscriptVar(" << var->toString() << "[" << index->toString() << "])";
    }
    return oss.str();
}

std::string RecordType::toString() const {
    std::ostringstream oss;
    oss << "RecordType({";
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << fields[i]->toString();
    }
    oss << "})";
    return oss.str();
}

std::string TypeDecl::toString() const {
    std::ostringstream oss;
    oss << "TypeDecl(" << name << " = " << type->toString() << ")";
    return oss.str();
}

std::string VarDecl::toString() const {
    std::ostringstream oss;
    oss << "VarDecl(" << name;
    if (!type_id.empty()) {
        oss << ": " << type_id;
    }
    oss << " := " << init->toString() << ")";
    return oss.str();
}

std::string FunctionDecl::toString() const {
    std::ostringstream oss;
    oss << "FunctionDecl(" << name << ", params=[";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << params[i]->toString();
    }
    oss << "], body=" << body->toString() << ")";
    return oss.str();
}

std::string CallExpr::toString() const {
    std::ostringstream oss;
    oss << "Call(" << func << ", [";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << args[i]->toString();
    }
    oss << "])";
    return oss.str();
}

std::string OpExpr::toString() const {
    std::ostringstream oss;
    oss << "Op(" << left->toString() << " " << opToString(oper) << " " << right->toString() << ")";
    return oss.str();
}

std::string RecordExpr::toString() const {
    std::ostringstream oss;
    oss << "Record(" << type_id << ", {";
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << fields[i].first << "=" << fields[i].second->toString();
    }
    oss << "})";
    return oss.str();
}

std::string ArrayExpr::toString() const {
    std::ostringstream oss;
    oss << "Array(" << type_id << "[" << size->toString() << "] of " << init->toString() << ")";
    return oss.str();
}

std::string AssignExpr::toString() const {
    std::ostringstream oss;
    oss << "Assign(" << var->toString() << " := " << expr->toString() << ")";
    return oss.str();
}

std::string IfExpr::toString() const {
    std::ostringstream oss;
    oss << "If(" << test->toString() << " then " << then_clause->toString();
    if (else_clause) {
        oss << " else " << else_clause->toString();
    }
    oss << ")";
    return oss.str();
}

std::string WhileExpr::toString() const {
    std::ostringstream oss;
    oss << "While(" << test->toString() << " do " << body->toString() << ")";
    return oss.str();
}

std::string ForExpr::toString() const {
    std::ostringstream oss;
    oss << "For(" << var << " := " << lo->toString() << " to " << hi->toString() << " do "
        << body->toString() << ")";
    return oss.str();
}

std::string LetExpr::toString() const {
    std::ostringstream oss;
    oss << "Let({";
    for (size_t i = 0; i < decls.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << decls[i]->toString();
    }
    oss << "} in [";
    for (size_t i = 0; i < body.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << body[i]->toString();
    }
    oss << "])";
    return oss.str();
}

std::string SeqExpr::toString() const {
    std::ostringstream oss;
    oss << "Seq(";
    for (size_t i = 0; i < exprs.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << exprs[i]->toString();
    }
    oss << ")";
    return oss.str();
}

}  // namespace tiger
