#ifndef TIGER_TRANSLATE_TEMP_HPP
#define TIGER_TRANSLATE_TEMP_HPP

#include <string>

namespace tiger {
namespace translate {

/**
 * Temp - Abstract name for a local variable (temporary)
 */
class Temp {
public:
    explicit Temp(int id = 0) : id_(id) {}
    int id() const { return id_; }
    bool operator==(const Temp& other) const { return id_ == other.id_; }
    bool operator!=(const Temp& other) const { return id_ != other.id_; }
    bool operator<(const Temp& other) const { return id_ < other.id_; }
    std::string toString() const { return "t" + std::to_string(id_); }

private:
    int id_;
};

/**
 * Label - Abstract name for a static memory address
 */
class Label {
public:
    explicit Label(const std::string& name = "") : name_(name), id_(-1) {}
    explicit Label(int id) : name_("L" + std::to_string(id)), id_(id) {}
    const std::string& name() const { return name_; }
    int id() const { return id_; }
    bool operator==(const Label& other) const { return name_ == other.name_; }
    bool operator!=(const Label& other) const { return name_ != other.name_; }
    std::string toString() const { return name_; }

private:
    std::string name_;
    int id_;
};

/**
 * TempFactory - Factory for creating unique temporaries and labels
 */
class TempFactory {
public:
    TempFactory() : tempCounter_(0), labelCounter_(0) {}
    Temp newTemp() { return Temp(tempCounter_++); }
    Label newLabel() { return Label(labelCounter_++); }
    Label namedLabel(const std::string& name) { return Label(name); }

private:
    int tempCounter_;
    int labelCounter_;
};

}  // namespace translate
}  // namespace tiger

namespace std {
template <>
struct hash<tiger::translate::Temp> {
    size_t operator()(const tiger::translate::Temp& t) const { return hash<int>()(t.id()); }
};
template <>
struct hash<tiger::translate::Label> {
    size_t operator()(const tiger::translate::Label& l) const { return hash<string>()(l.name()); }
};
}  // namespace std

#endif  // TIGER_TRANSLATE_TEMP_HPP
