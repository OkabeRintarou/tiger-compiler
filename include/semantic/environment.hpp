#pragma once

#include <memory>
#include <string>
#include <vector>

#include "semantic/symbol_table.hpp"
#include "semantic/types.hpp"

namespace tiger {
namespace semantic {

// Base class for value entries (variables and functions)
class ValueEntry {
   public:
    enum Kind { VAR, FUNC };

    virtual ~ValueEntry() = default;
    virtual Kind getKind() const = 0;

    bool isVar() const { return getKind() == VAR; }
    bool isFunc() const { return getKind() == FUNC; }
};

using ValueEntryPtr = std::shared_ptr<ValueEntry>;

// Entry for variables in symbol table
class VarEntry : public ValueEntry {
   private:
    TypePtr type_;
    bool readOnly_;  // For loop variables

   public:
    VarEntry(TypePtr type, bool readOnly = false) : type_(type), readOnly_(readOnly) {}

    Kind getKind() const override { return VAR; }
    TypePtr getType() const { return type_; }
    bool isReadOnly() const { return readOnly_; }
};

// Entry for functions in symbol table
class FuncEntry : public ValueEntry {
   private:
    std::vector<TypePtr> paramTypes_;
    TypePtr returnType_;

   public:
    FuncEntry(const std::vector<TypePtr>& params, TypePtr ret)
        : paramTypes_(params), returnType_(ret) {}

    Kind getKind() const override { return FUNC; }
    const std::vector<TypePtr>& getParamTypes() const { return paramTypes_; }
    TypePtr getReturnType() const { return returnType_; }
    size_t getParamCount() const { return paramTypes_.size(); }
};

// Environment for semantic analysis
// Tiger has two separate namespaces:
//   1. Type namespace (for type declarations)
//   2. Value namespace (for variables AND functions - they share the same namespace)
class Environment {
   private:
    TypeContext& typeCtx_;                 // Type context (manages all types)
    SymbolTable<TypePtr> typeEnv_;         // Type namespace
    SymbolTable<ValueEntryPtr> valueEnv_;  // Value namespace (variables + functions)

    int loopDepth_;  // Track nesting depth of loops (for break statement)

   public:
    explicit Environment(TypeContext& ctx) : typeCtx_(ctx), loopDepth_(0) { initBuiltins(); }

    // Get type context
    TypeContext& getTypeContext() { return typeCtx_; }

    // Scope management
    void beginScope() {
        typeEnv_.beginScope();
        valueEnv_.beginScope();
    }

    void endScope() {
        typeEnv_.endScope();
        valueEnv_.endScope();
    }

    // Type operations
    void enterType(const std::string& name, TypePtr type) { typeEnv_.enter(name, type); }

    TypePtr lookupType(const std::string& name) const { return typeEnv_.lookup(name); }

    bool typeExistsInCurrentScope(const std::string& name) const {
        return typeEnv_.existsInCurrentScope(name);
    }

    // Value operations (unified for both variables and functions)
    void enterValue(const std::string& name, ValueEntryPtr entry) { valueEnv_.enter(name, entry); }

    ValueEntryPtr lookupValue(const std::string& name) const { return valueEnv_.lookup(name); }

    bool valueExistsInCurrentScope(const std::string& name) const {
        return valueEnv_.existsInCurrentScope(name);
    }

    // Convenience methods for variables
    void enterVar(const std::string& name, TypePtr type, bool readOnly = false) {
        enterValue(name, std::make_shared<VarEntry>(type, readOnly));
    }

    VarEntry* lookupVar(const std::string& name) const {
        auto entry = lookupValue(name);
        if (entry && entry->isVar()) {
            return static_cast<VarEntry*>(entry.get());
        }
        return nullptr;
    }

    // Convenience methods for functions
    void enterFunc(const std::string& name, const std::vector<TypePtr>& params, TypePtr ret) {
        enterValue(name, std::make_shared<FuncEntry>(params, ret));
    }

    FuncEntry* lookupFunc(const std::string& name) const {
        auto entry = lookupValue(name);
        if (entry && entry->isFunc()) {
            return static_cast<FuncEntry*>(entry.get());
        }
        return nullptr;
    }

    // Loop management (for break statement validation)
    void enterLoop() { ++loopDepth_; }
    void exitLoop() { --loopDepth_; }
    bool inLoop() const { return loopDepth_ > 0; }

   private:
    // Initialize built-in types and functions
    void initBuiltins() {
        // Built-in types
        enterType("int", typeCtx_.getIntType());
        enterType("string", typeCtx_.getStringType());

        // Built-in functions with their signatures
        auto intTy = typeCtx_.getIntType();
        auto stringTy = typeCtx_.getStringType();
        auto voidTy = typeCtx_.getVoidType();

        // print(s: string)
        enterFunc("print", {stringTy}, voidTy);

        // printi(i: int)
        enterFunc("printi", {intTy}, voidTy);

        // flush()
        enterFunc("flush", {}, voidTy);

        // getchar() : string
        enterFunc("getchar", {}, stringTy);

        // ord(s: string) : int
        enterFunc("ord", {stringTy}, intTy);

        // chr(i: int) : string
        enterFunc("chr", {intTy}, stringTy);

        // size(s: string) : int
        enterFunc("size", {stringTy}, intTy);

        // substring(s: string, first: int, n: int) : string
        enterFunc("substring", {stringTy, intTy, intTy}, stringTy);

        // concat(s1: string, s2: string) : string
        enterFunc("concat", {stringTy, stringTy}, stringTy);

        // not(i: int) : int
        enterFunc("not", {intTy}, intTy);

        // exit(i: int)
        enterFunc("exit", {intTy}, voidTy);
    }
};

}  // namespace semantic
}  // namespace tiger
