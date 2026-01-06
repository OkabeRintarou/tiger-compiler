#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tiger {

// Generic symbol table template with scope nesting
// Supports enter/lookup operations with lexical scoping
template <typename T>
class SymbolTable {
private:
    // Each scope is a hash table mapping names to values
    std::vector<std::unordered_map<std::string, T>> scopes_;

public:
    SymbolTable() {
        // Start with a global scope
        beginScope();
    }

    // Enter a new scope
    void beginScope() { scopes_.emplace_back(); }

    // Exit current scope
    void endScope() {
        if (!scopes_.empty()) {
            scopes_.pop_back();
        }
    }

    // Add binding in current scope
    void enter(const std::string& name, const T& value) {
        if (!scopes_.empty()) {
            scopes_.back()[name] = value;
        }
    }

    // Look up binding (searches from inner to outer scopes)
    T lookup(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return found->second;
            }
        }
        return T();  // Return default value if not found
    }

    // Check if name exists in current scope only
    bool existsInCurrentScope(const std::string& name) const {
        if (scopes_.empty()) return false;
        return scopes_.back().find(name) != scopes_.back().end();
    }

    // Check if name exists in any scope
    bool exists(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            if (it->find(name) != it->end()) {
                return true;
            }
        }
        return false;
    }

    size_t scopeDepth() const { return scopes_.size(); }
};

}  // namespace tiger
