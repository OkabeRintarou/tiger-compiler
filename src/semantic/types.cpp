#include "semantic/types.hpp"

namespace tiger {
namespace semantic {

// Safe downcasting methods
IntType* Type::asInt() {
    if (isInt()) return static_cast<IntType*>(actual());
    if (isName() && actual() && actual()->isInt()) return static_cast<IntType*>(actual()->actual());
    return nullptr;
}

StringType* Type::asString() {
    if (isString()) return static_cast<StringType*>(actual());
    if (isName() && actual() && actual()->isString())
        return static_cast<StringType*>(actual()->actual());
    return nullptr;
}

RecordType* Type::asRecord() {
    if (isRecord()) return static_cast<RecordType*>(actual());
    if (isName() && actual() && actual()->isRecord())
        return static_cast<RecordType*>(actual()->actual());
    return nullptr;
}

ArrayType* Type::asArray() {
    if (isArray()) return static_cast<ArrayType*>(actual());
    if (isName() && actual() && actual()->isArray())
        return static_cast<ArrayType*>(actual()->actual());
    return nullptr;
}

NameType* Type::asName() { return isName() ? static_cast<NameType*>(this) : nullptr; }

const IntType* Type::asInt() const {
    if (isInt()) return static_cast<const IntType*>(actual());
    if (isName() && actual() && actual()->isInt())
        return static_cast<const IntType*>(actual()->actual());
    return nullptr;
}

const StringType* Type::asString() const {
    if (isString()) return static_cast<const StringType*>(actual());
    if (isName() && actual() && actual()->isString())
        return static_cast<const StringType*>(actual()->actual());
    return nullptr;
}

const RecordType* Type::asRecord() const {
    if (isRecord()) return static_cast<const RecordType*>(actual());
    if (isName() && actual() && actual()->isRecord())
        return static_cast<const RecordType*>(actual()->actual());
    return nullptr;
}

const ArrayType* Type::asArray() const {
    if (isArray()) return static_cast<const ArrayType*>(actual());
    if (isName() && actual() && actual()->isArray())
        return static_cast<const ArrayType*>(actual()->actual());
    return nullptr;
}

const NameType* Type::asName() const {
    return isName() ? static_cast<const NameType*>(this) : nullptr;
}

std::string RecordType::toString() const {
    std::string result = "{";
    for (size_t i = 0; i < fields_.size(); ++i) {
        if (i > 0) result += ", ";
        result += fields_[i].name + ": " + fields_[i].type->toString();
    }
    result += "}";
    return result;
}

std::string NameType::toString() const {
    if (!binding_) {
        return name_ + "=(unbinding)";
    }
    const Type* actual = binding_->actual();
    if (!actual) {
        return name_ + "=(null)";
    }

    // Check for cycle (if actual type is the same as this NameType)
    if (actual->isName() && static_cast<const NameType*>(actual)->equals(this)) {
        return name_ + "=...";
    }

    if (actual->isRecord()) {
        const RecordType* record = actual->asRecord();
        const auto& fields = record->getFields();
        std::string result = "{";

        for (size_t i = 0, e = fields.size(); i < e; i++) {
            if (i > 0) result += ", ";
            result += fields[i].name + ": ";

            if (auto nameType = fields[i].type->asName()) {
                result += nameType->getName();
            } else {
                result += fields[i].type->toString();
            }
        }

        result += "}";
        return name_ + "=(" + result + ")";
    } else {
        return name_ + "=(" + actual->toString() + ")";
    }
}

std::string FunctionType::toString() const {
    std::string result = "(";
    for (size_t i = 0; i < paramTypes_.size(); ++i) {
        if (i > 0) result += ", ";
        result += paramTypes_[i]->toString();
    }
    result += ") -> " + returnType_->toString();
    return result;
}

// Default equality implementation
bool Type::equals(const Type* other) const {
    if (!other) return false;
    return actual() == other->actual();
}

// Type compatibility check
bool isCompatible(Type* t1, Type* t2) {
    if (!t1 || !t2) return false;

    Type* a1 = t1->actual();
    Type* a2 = t2->actual();

    // Nil is compatible with any record type
    if (a1->isNil() && a2->isRecord()) return true;
    if (a2->isNil() && a1->isRecord()) return true;

    // Use type-specific equals method
    return a1->equals(a2);
}

}  // namespace semantic
}  // namespace tiger
