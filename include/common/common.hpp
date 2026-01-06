#ifndef TIGER_COMMON_HPP
#define TIGER_COMMON_HPP

#include <stdexcept>
#include <string>

namespace tiger {

// Base error class for all Tiger compiler errors
class TigerError : public std::runtime_error {
public:
    explicit TigerError(const std::string& msg) : std::runtime_error(msg) {}
};

// Lexical error
class LexicalError : public TigerError {
public:
    LexicalError(const std::string& msg, int line, int column)
        : TigerError(msg), line_(line), column_(column) {}

    int line() const { return line_; }
    int column() const { return column_; }

private:
    int line_;
    int column_;
};

// Syntax error
class SyntaxError : public TigerError {
public:
    SyntaxError(const std::string& msg, int line, int column)
        : TigerError(msg), line_(line), column_(column) {}

    int line() const { return line_; }
    int column() const { return column_; }

private:
    int line_;
    int column_;
};

}  // namespace tiger

#endif  // TIGER_COMMON_HPP
