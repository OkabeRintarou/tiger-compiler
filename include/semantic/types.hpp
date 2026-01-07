#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tiger {
namespace semantic {

// Forward declarations
class Type;
class IntType;
class StringType;
class NilType;
class VoidType;
class RecordType;
class ArrayType;
class NameType;
class FunctionType;
class TypeContext;

using TypePtr = std::shared_ptr<Type>;

// Base class for all types in Tiger type system
class Type {
public:
    enum Kind {
        INT,
        STRING,
        NIL,
        VOID,
        RECORD,
        ARRAY,
        NAME,  // Type alias that needs resolution
        FUNCTION
    };

protected:
    Kind kind_;

public:
    explicit Type(Kind k) : kind_(k) {}
    virtual ~Type() = default;

    Kind getKind() const { return kind_; }

    // Type checking predicates
    bool isInt() const { return kind_ == INT; }
    bool isString() const { return kind_ == STRING; }
    bool isNil() const { return kind_ == NIL; }
    bool isVoid() const { return kind_ == VOID; }
    bool isRecord() const { return kind_ == RECORD; }
    bool isArray() const { return kind_ == ARRAY; }
    bool isName() const { return kind_ == NAME; }
    bool isFunction() const { return kind_ == FUNCTION; }

    // Safe downcasting
    IntType* asInt();
    StringType* asString();
    RecordType* asRecord();
    ArrayType* asArray();
    NameType* asName();

    // Get actual type (resolve NameType aliases)
    virtual Type* actual() { return this; }

    // Type equality comparison
    virtual bool equals(Type* other);

    // Print type information
    virtual std::string toString() const = 0;
};

// Integer type
class IntType : public Type {
public:
    IntType() : Type(INT) {}
    std::string toString() const override { return "int"; }
};

// String type
class StringType : public Type {
public:
    StringType() : Type(STRING) {}
    std::string toString() const override { return "string"; }
};

// Nil type (can only be assigned to record types)
class NilType : public Type {
public:
    NilType() : Type(NIL) {}
    std::string toString() const override { return "nil"; }
};

// Void type (for functions with no return value)
class VoidType : public Type {
public:
    VoidType() : Type(VOID) {}
    std::string toString() const override { return "void"; }
};

// Field in a record type
struct RecordField {
    std::string name;
    TypePtr type;

    RecordField(const std::string& n, TypePtr t) : name(n), type(t) {}
};

// Record type (struct)
class RecordType : public Type {
private:
    std::vector<RecordField> fields_;
    int id_;  // Unique identifier to distinguish different record definitions

public:
    explicit RecordType(int id) : Type(RECORD), id_(id) {}

    void addField(const std::string& name, TypePtr type) { fields_.emplace_back(name, type); }

    void setFields(const std::vector<RecordField>& fields) { fields_ = fields; }

    const std::vector<RecordField>& getFields() const { return fields_; }

    // Look up field type by name
    TypePtr getFieldType(const std::string& name) const {
        for (const auto& field : fields_) {
            if (field.name == name) return field.type;
        }
        return nullptr;
    }

    int getId() const { return id_; }

    bool equals(Type* other) override {
        if (other->getKind() == NIL) return true;  // nil can be assigned to any record
        if (other->getKind() != RECORD) return false;
        return id_ == static_cast<RecordType*>(other->actual())->id_;
    }

    std::string toString() const override {
        std::string result = "{";
        for (size_t i = 0; i < fields_.size(); ++i) {
            if (i > 0) result += ", ";
            result += fields_[i].name + ": " + fields_[i].type->toString();
        }
        result += "}";
        return result;
    }
};

// Array type
class ArrayType : public Type {
private:
    TypePtr elementType_;
    int id_;  // Unique identifier

public:
    ArrayType(TypePtr elemType, int id) : Type(ARRAY), elementType_(elemType), id_(id) {}

    TypePtr getElementType() const { return elementType_; }
    int getId() const { return id_; }

    bool equals(Type* other) override {
        if (other->getKind() != ARRAY) return false;
        return id_ == static_cast<ArrayType*>(other->actual())->id_;
    }

    std::string toString() const override { return "array of " + elementType_->toString(); }
};

// Named type (type alias that needs to be resolved)
class NameType : public Type {
private:
    std::string name_;
    mutable TypePtr binding_;  // Bound actual type (lazy resolution)

public:
    explicit NameType(const std::string& name) : Type(NAME), name_(name), binding_(nullptr) {}

    const std::string& getName() const { return name_; }

    void bind(TypePtr type) { binding_ = type; }
    TypePtr getBinding() const { return binding_; }

    Type* actual() override {
        if (!binding_) return this;
        Type* t = binding_.get();
        // Recursively resolve type alias chains
        while (t && t->isName()) {
            TypePtr next = static_cast<NameType*>(t)->getBinding();
            if (!next) break;
            t = next.get();
        }
        return t;
    }

    bool equals(Type* other) override {
        Type* act = actual();
        if (act == this) return false;  // Unresolved type
        return act->equals(other->actual());
    }

    std::string toString() const override {
        if (binding_) {
            return name_ + " (= " + binding_->toString() + ")";
        }
        return name_;
    }
};

// Function type
class FunctionType : public Type {
private:
    std::vector<TypePtr> paramTypes_;
    TypePtr returnType_;

public:
    FunctionType(const std::vector<TypePtr>& params, TypePtr ret)
        : Type(FUNCTION), paramTypes_(params), returnType_(ret) {}

    const std::vector<TypePtr>& getParamTypes() const { return paramTypes_; }
    TypePtr getReturnType() const { return returnType_; }

    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < paramTypes_.size(); ++i) {
            if (i > 0) result += ", ";
            result += paramTypes_[i]->toString();
        }
        result += ") -> " + returnType_->toString();
        return result;
    }
};

// Type context - manages all type instances
class TypeContext {
private:
    // Primitive types (shared instances)
    std::shared_ptr<IntType> intType_;
    std::shared_ptr<StringType> stringType_;
    std::shared_ptr<NilType> nilType_;
    std::shared_ptr<VoidType> voidType_;

    // ID counters for unique types
    int nextRecordId_;
    int nextArrayId_;

    // Note on Tiger vs LLVM type systems:
    // - LLVM uses structural typing: ArrayType::get() caches and returns the same
    //   instance for arrays with the same element type (e.g., all [10 x i32] share one instance)
    // - Tiger uses nominal typing: each "type arr = array of int" declaration
    //   creates a DISTINCT type, even if the structure is identical
    // Therefore, we DON't cache array/record types - each declaration gets a unique ID

public:
    TypeContext()
        : intType_(std::make_shared<IntType>()),
          stringType_(std::make_shared<StringType>()),
          nilType_(std::make_shared<NilType>()),
          voidType_(std::make_shared<VoidType>()),
          nextRecordId_(0),
          nextArrayId_(0) {}

    // Get primitive types
    TypePtr getIntType() const { return intType_; }
    TypePtr getStringType() const { return stringType_; }
    TypePtr getNilType() const { return nilType_; }
    TypePtr getVoidType() const { return voidType_; }

    // Create new record type (each declaration creates a unique type)
    std::shared_ptr<RecordType> createRecordType() {
        return std::make_shared<RecordType>(nextRecordId_++);
    }

    // Create new array type (each declaration creates a unique type)
    std::shared_ptr<ArrayType> createArrayType(TypePtr elemType) {
        return std::make_shared<ArrayType>(elemType, nextArrayId_++);
    }

    // Create named type (type alias)
    std::shared_ptr<NameType> createNameType(const std::string& name) {
        return std::make_shared<NameType>(name);
    }

    // Create function type
    std::shared_ptr<FunctionType> createFunctionType(const std::vector<TypePtr>& params,
                                                     TypePtr ret) {
        return std::make_shared<FunctionType>(params, ret);
    }
};

// Check type compatibility
bool isCompatible(Type* t1, Type* t2);

}  // namespace semantic
}  // namespace tiger
