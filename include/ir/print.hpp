#ifndef TIGER_IR_PRINT_HPP
#define TIGER_IR_PRINT_HPP

#include <ostream>
#include <string>

#include "ir/tree.hpp"

namespace tiger {
namespace ir {

/**
 * TreePrinter - Pretty print IR trees with indentation
 */
class TreePrinter {
public:
    explicit TreePrinter(std::ostream& os, int indentSize = 2)
        : os_(os), indentSize_(indentSize), indent_(0) {}

    void print(const ExpPtr& exp);
    void print(const StmPtr& stm);

private:
    std::ostream& os_;
    int indentSize_;
    int indent_;

    void printIndent();
    void incIndent() { indent_ += indentSize_; }
    void decIndent() { indent_ -= indentSize_; }

    // Expression printers
    void printExp(const ConstExp* exp);
    void printExp(const NameExp* exp);
    void printExp(const TempExp* exp);
    void printExp(const BinOpExp* exp);
    void printExp(const MemExp* exp);
    void printExp(const CallExp* exp);
    void printExp(const EseqExp* exp);

    // Statement printers
    void printStm(const MoveStm* stm);
    void printStm(const ExpStm* stm);
    void printStm(const JumpStm* stm);
    void printStm(const CJumpStm* stm);
    void printStm(const SeqStm* stm);
    void printStm(const LabelStm* stm);

    const char* binOpStr(BinOp op);
    const char* relOpStr(RelOp op);
};

// Convenience function
inline void printTree(std::ostream& os, const StmPtr& stm) {
    TreePrinter printer(os);
    printer.print(stm);
}

inline void printTree(std::ostream& os, const ExpPtr& exp) {
    TreePrinter printer(os);
    printer.print(exp);
}

}  // namespace ir
}  // namespace tiger

#endif  // TIGER_IR_PRINT_HPP
