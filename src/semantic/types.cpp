#include "semantic/types.hpp"

namespace tiger {
namespace semantic {

// Safe downcasting methods
IntType* Type::asInt() { return isInt() ? static_cast<IntType*>(actual()) : nullptr; }

StringType* Type::asString() { return isString() ? static_cast<StringType*>(actual()) : nullptr; }

RecordType* Type::asRecord() { return isRecord() ? static_cast<RecordType*>(actual()) : nullptr; }

ArrayType* Type::asArray() { return isArray() ? static_cast<ArrayType*>(actual()) : nullptr; }

NameType* Type::asName() { return isName() ? static_cast<NameType*>(this) : nullptr; }

// Default equality implementation
bool Type::equals(Type* other) {
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
