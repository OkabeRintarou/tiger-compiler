#ifndef TIGER_AST_HPP
#define TIGER_AST_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/common.hpp"

namespace tiger {
namespace ast {

// Forward declarations
class Expr;
class Decl;
class Type;
class Field;
class VarExpr;
class NilExpr;
class IntExpr;
class StringExpr;
class CallExpr;
class OpExpr;
class RecordExpr;
class ArrayExpr;
class AssignExpr;
class IfExpr;
class WhileExpr;
class ForExpr;
class BreakExpr;
class LetExpr;
class SeqExpr;
class TypeDecl;
class VarDecl;
class FunctionDecl;
class NameType;
class RecordType;
class ArrayType;

// Type aliases
using ExprPtr = std::shared_ptr<Expr>;
using DeclPtr = std::shared_ptr<Decl>;
using TypeDeclPtr = std::shared_ptr<TypeDecl>;
using FunctionDeclPtr = std::shared_ptr<FunctionDecl>;
using TypePtr = std::shared_ptr<Type>;
using FieldPtr = std::shared_ptr<Field>;
using ExprList = std::vector<ExprPtr>;
using DeclList = std::vector<DeclPtr>;
using FieldList = std::vector<FieldPtr>;

// ========== Visitor Pattern ==========

template <typename ResultType = void>
class Visitor;

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;
};

template <typename ResultType>
class Visitor {
public:
    virtual ~Visitor() = default;

    virtual ResultType visit(VarExpr* expr) = 0;
    virtual ResultType visit(NilExpr* expr) = 0;
    virtual ResultType visit(IntExpr* expr) = 0;
    virtual ResultType visit(StringExpr* expr) = 0;
    virtual ResultType visit(CallExpr* expr) = 0;
    virtual ResultType visit(OpExpr* expr) = 0;
    virtual ResultType visit(RecordExpr* expr) = 0;
    virtual ResultType visit(ArrayExpr* expr) = 0;
    virtual ResultType visit(AssignExpr* expr) = 0;
    virtual ResultType visit(IfExpr* expr) = 0;
    virtual ResultType visit(WhileExpr* expr) = 0;
    virtual ResultType visit(ForExpr* expr) = 0;
    virtual ResultType visit(BreakExpr* expr) = 0;
    virtual ResultType visit(LetExpr* expr) = 0;
    virtual ResultType visit(SeqExpr* expr) = 0;

    virtual ResultType visit(TypeDecl* decl) = 0;
    virtual ResultType visit(VarDecl* decl) = 0;
    virtual ResultType visit(FunctionDecl* decl) = 0;

    virtual ResultType visit(NameType* type) = 0;
    virtual ResultType visit(RecordType* type) = 0;
    virtual ResultType visit(ArrayType* type) = 0;
};

// ========== Types ==========

class Type : public ASTNode {
public:
    enum class Kind { NAME, RECORD, ARRAY };
    Kind kind;
    explicit Type(Kind k) : kind(k) {}

    template <typename ResultType = void>
    ResultType accept(Visitor<ResultType>& visitor);
};

class NameType : public Type {
public:
    std::string name;
    NameType(const std::string& n) : Type(Kind::NAME), name(n) {}
    std::string toString() const override { return "NameType(" + name + ")"; }
};

class RecordType : public Type {
public:
    FieldList fields;
    RecordType() : Type(Kind::RECORD) {}
    std::string toString() const override;
};

class ArrayType : public Type {
public:
    std::string element_type;
    ArrayType(const std::string& et) : Type(Kind::ARRAY), element_type(et) {}
    std::string toString() const override { return "ArrayType(" + element_type + ")"; }
};

// ========== Fields ==========

class Field : public ASTNode {
public:
    std::string name;
    std::string type_id;
    bool escape;

    Field(const std::string& n, const std::string& t, bool e = false)
        : name(n), type_id(t), escape(e) {}

    std::string toString() const override { return "Field(" + name + ": " + type_id + ")"; }
};

// ========== Declarations ==========

class Decl : public ASTNode {
public:
    enum class Kind { TYPE, VAR, FUNCTION };
    Kind kind;
    explicit Decl(Kind k) : kind(k) {}

    bool isTypeDecl() const { return kind == Kind::TYPE; }
    bool isVarDecl() const { return kind == Kind::VAR; }
    bool isFunctionDecl() const { return kind == Kind::FUNCTION; }

    template <typename ResultType = void>
    ResultType accept(Visitor<ResultType>& visitor);
};

class TypeDecl : public Decl {
public:
    std::string name;
    TypePtr type;
    TypeDecl(const std::string& n, TypePtr t) : Decl(Kind::TYPE), name(n), type(t) {}
    std::string toString() const override;
};

class VarDecl : public Decl {
public:
    std::string name;
    std::string type_id;
    ExprPtr init;
    bool escape;
    VarDecl(const std::string& n, const std::string& tid, ExprPtr i)
        : Decl(Kind::VAR), name(n), type_id(tid), init(i), escape(false) {}
    std::string toString() const override;
};

class FunctionDecl : public Decl {
public:
    std::string name;
    FieldList params;
    std::string result_type;
    ExprPtr body;
    FunctionDecl(const std::string& n, const FieldList& p, const std::string& rt, ExprPtr b)
        : Decl(Kind::FUNCTION), name(n), params(p), result_type(rt), body(b) {}
    std::string toString() const override;
};

// ========== Expressions ==========

class Expr : public ASTNode {
public:
    enum class Kind {
        VAR,
        NIL,
        INT,
        STRING,
        CALL,
        OP,
        RECORD,
        ARRAY,
        ASSIGN,
        IF,
        WHILE,
        FOR,
        BREAK,
        LET,
        SEQ
    };
    Kind kind;
    explicit Expr(Kind k) : kind(k) {}

    template <typename ResultType = void>
    ResultType accept(Visitor<ResultType>& visitor);
};

class VarExpr : public Expr {
public:
    enum class VarKind { SIMPLE, FIELD, SUBSCRIPT };
    VarKind var_kind;
    std::string name;  // For SIMPLE: variable name; for FIELD: field name; for SUBSCRIPT: unused
    ExprPtr var;       // For FIELD/SUBSCRIPT: left side expression
    ExprPtr index;     // For SUBSCRIPT: index expression

    VarExpr(const std::string& n) : Expr(Kind::VAR), var_kind(VarKind::SIMPLE), name(n) {}
    std::string toString() const override;
};

class NilExpr : public Expr {
public:
    NilExpr() : Expr(Kind::NIL) {}
    std::string toString() const override { return "Nil()"; }
};

class IntExpr : public Expr {
public:
    int value;
    explicit IntExpr(int v) : Expr(Kind::INT), value(v) {}
    std::string toString() const override { return "Int(" + std::to_string(value) + ")"; }
};

class StringExpr : public Expr {
public:
    std::string value;
    explicit StringExpr(const std::string& v) : Expr(Kind::STRING), value(v) {}
    std::string toString() const override { return "String(\"" + value + "\")"; }
};

class CallExpr : public Expr {
public:
    std::string func;
    ExprList args;
    CallExpr(const std::string& f, const ExprList& a) : Expr(Kind::CALL), func(f), args(a) {}
    std::string toString() const override;
};

class OpExpr : public Expr {
public:
    enum class Op { PLUS, MINUS, TIMES, DIVIDE, EQ, NEQ, LT, GT, LE, GE, AND, OR };
    Op oper;
    ExprPtr left;
    ExprPtr right;
    OpExpr(Op o, ExprPtr l, ExprPtr r) : Expr(Kind::OP), oper(o), left(l), right(r) {}
    std::string toString() const override;
};

class RecordExpr : public Expr {
public:
    std::string type_id;
    std::vector<std::pair<std::string, ExprPtr>> fields;
    RecordExpr(const std::string& tid, const std::vector<std::pair<std::string, ExprPtr>>& f)
        : Expr(Kind::RECORD), type_id(tid), fields(f) {}
    std::string toString() const override;
};

class ArrayExpr : public Expr {
public:
    std::string type_id;
    ExprPtr size;
    ExprPtr init;
    ArrayExpr(const std::string& tid, ExprPtr s, ExprPtr i)
        : Expr(Kind::ARRAY), type_id(tid), size(s), init(i) {}
    std::string toString() const override;
};

class AssignExpr : public Expr {
public:
    ExprPtr var;
    ExprPtr expr;
    AssignExpr(ExprPtr v, ExprPtr e) : Expr(Kind::ASSIGN), var(v), expr(e) {}
    std::string toString() const override;
};

class IfExpr : public Expr {
public:
    ExprPtr test;
    ExprPtr then_clause;
    ExprPtr else_clause;
    IfExpr(ExprPtr t, ExprPtr th, ExprPtr el = nullptr)
        : Expr(Kind::IF), test(t), then_clause(th), else_clause(el) {}
    std::string toString() const override;
};

class WhileExpr : public Expr {
public:
    ExprPtr test;
    ExprPtr body;
    WhileExpr(ExprPtr t, ExprPtr b) : Expr(Kind::WHILE), test(t), body(b) {}
    std::string toString() const override;
};

class ForExpr : public Expr {
public:
    std::string var;
    ExprPtr lo;
    ExprPtr hi;
    ExprPtr body;
    bool escape;
    ForExpr(const std::string& v, ExprPtr low, ExprPtr high, ExprPtr b)
        : Expr(Kind::FOR), var(v), lo(low), hi(high), body(b), escape(false) {}
    std::string toString() const override;
};

class BreakExpr : public Expr {
public:
    BreakExpr() : Expr(Kind::BREAK) {}
    std::string toString() const override { return "Break()"; }
};

class LetExpr : public Expr {
public:
    DeclList decls;
    ExprList body;
    LetExpr(const DeclList& d, const ExprList& b) : Expr(Kind::LET), decls(d), body(b) {}
    std::string toString() const override;
};

class SeqExpr : public Expr {
public:
    ExprList exprs;
    explicit SeqExpr(const ExprList& e) : Expr(Kind::SEQ), exprs(e) {}
    std::string toString() const override;
};

// ========== Utility Functions ==========

std::string opToString(OpExpr::Op op);

// ========== Visitor Template Implementations ==========

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

}  // namespace ast
}  // namespace tiger

#endif  // TIGER_AST_HPP
