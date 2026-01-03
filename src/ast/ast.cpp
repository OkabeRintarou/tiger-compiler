#include "ast/ast.hpp"

#include <sstream>

namespace tiger {

// ========== Visitor Implementations ==========

template <typename ResultType>
ResultType Expr::accept(Visitor<ResultType>& visitor) {
    switch (kind) {
        case Kind::VAR:
            return visitor.visit(static_cast<VarExpr*>(this));
        case Kind::NIL:
            return visitor.visit(static_cast<NilExpr*>(this));
        case Kind::INT:
            return visitor.visit(static_cast<IntExpr*>(this));
        case Kind::STRING:
            return visitor.visit(static_cast<StringExpr*>(this));
        case Kind::CALL:
            return visitor.visit(static_cast<CallExpr*>(this));
        case Kind::OP:
            return visitor.visit(static_cast<OpExpr*>(this));
        case Kind::RECORD:
            return visitor.visit(static_cast<RecordExpr*>(this));
        case Kind::ARRAY:
            return visitor.visit(static_cast<ArrayExpr*>(this));
        case Kind::ASSIGN:
            return visitor.visit(static_cast<AssignExpr*>(this));
        case Kind::IF:
            return visitor.visit(static_cast<IfExpr*>(this));
        case Kind::WHILE:
            return visitor.visit(static_cast<WhileExpr*>(this));
        case Kind::FOR:
            return visitor.visit(static_cast<ForExpr*>(this));
        case Kind::BREAK:
            return visitor.visit(static_cast<BreakExpr*>(this));
        case Kind::LET:
            return visitor.visit(static_cast<LetExpr*>(this));
        case Kind::SEQ:
            return visitor.visit(static_cast<SeqExpr*>(this));
        default:
            throw std::runtime_error("Unknown expression kind");
    }
}

template <typename ResultType>
ResultType Type::accept(Visitor<ResultType>& visitor) {
    switch (kind) {
        case Kind::NAME:
            return visitor.visit(static_cast<NameType*>(this));
        case Kind::RECORD:
            return visitor.visit(static_cast<RecordType*>(this));
        case Kind::ARRAY:
            return visitor.visit(static_cast<ArrayType*>(this));
        default:
            throw std::runtime_error("Unknown type kind");
    }
}

template <typename ResultType>
ResultType Decl::accept(Visitor<ResultType>& visitor) {
    switch (kind) {
        case Kind::TYPE:
            return visitor.visit(static_cast<TypeDecl*>(this));
        case Kind::VAR:
            return visitor.visit(static_cast<VarDecl*>(this));
        case Kind::FUNCTION:
            return visitor.visit(static_cast<FunctionDecl*>(this));
        default:
            throw std::runtime_error("Unknown declaration kind");
    }
}

// Explicit template instantiations
template void Expr::accept<void>(Visitor<void>&);
template int Expr::accept<int>(Visitor<int>&);
template std::string Expr::accept<std::string>(Visitor<std::string>&);

template void Type::accept<void>(Visitor<void>&);
template int Type::accept<int>(Visitor<int>&);
template std::string Type::accept<std::string>(Visitor<std::string>&);

template void Decl::accept<void>(Visitor<void>&);
template int Decl::accept<int>(Visitor<int>&);
template std::string Decl::accept<std::string>(Visitor<std::string>&);

// ========== AST toString Methods ==========

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
    return "TypeDecl(" + name + ", " + type->toString() + ")";
}

std::string VarDecl::toString() const {
    std::ostringstream oss;
    oss << "VarDecl(" << name;
    if (!type_id.empty()) oss << ": " << type_id;
    oss << ", " << init->toString() << ")";
    return oss.str();
}

std::string FunctionDecl::toString() const {
    std::ostringstream oss;
    oss << "FunctionDecl(" << name << ", [";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << params[i]->toString();
    }
    oss << "]";
    if (!result_type.empty()) oss << ": " << result_type;
    oss << ", " << body->toString() << ")";
    return oss.str();
}

std::string VarExpr::toString() const {
    switch (var_kind) {
        case VarKind::SIMPLE:
            return "Var(" + name + ")";
        case VarKind::FIELD:
            return "FieldVar(" + var->toString() + "." + field + ")";
        case VarKind::SUBSCRIPT:
            return "SubscriptVar(" + var->toString() + "[" + index->toString() + "])";
    }
    return "";
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
    }
    return "?";
}

std::string OpExpr::toString() const {
    return "Op(" + left->toString() + " " + opToString(oper) + " " + right->toString() + ")";
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
    return "Array(" + type_id + ", " + size->toString() + ", " + init->toString() + ")";
}

std::string AssignExpr::toString() const {
    return "Assign(" + var->toString() + " := " + expr->toString() + ")";
}

std::string IfExpr::toString() const {
    std::ostringstream oss;
    oss << "If(" << test->toString() << ", " << then_clause->toString();
    if (else_clause) oss << ", " << else_clause->toString();
    oss << ")";
    return oss.str();
}

std::string WhileExpr::toString() const {
    return "While(" + test->toString() + ", " + body->toString() + ")";
}

std::string ForExpr::toString() const {
    return "For(" + var + " := " + lo->toString() + " to " + hi->toString() + ", " +
           body->toString() + ")";
}

std::string LetExpr::toString() const {
    std::ostringstream oss;
    oss << "Let([";
    for (size_t i = 0; i < decls.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << decls[i]->toString();
    }
    oss << "], [";
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
