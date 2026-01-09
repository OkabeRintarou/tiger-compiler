#include "ir/print.hpp"

namespace tiger {
namespace ir {

const char* TreePrinter::binOpStr(BinOp op) {
    switch (op) {
        case BinOp::PLUS:
            return "PLUS";
        case BinOp::MINUS:
            return "MINUS";
        case BinOp::MUL:
            return "MUL";
        case BinOp::DIV:
            return "DIV";
        case BinOp::AND:
            return "AND";
        case BinOp::OR:
            return "OR";
        case BinOp::XOR:
            return "XOR";
        case BinOp::LSHIFT:
            return "LSHIFT";
        case BinOp::RSHIFT:
            return "RSHIFT";
        case BinOp::ARSHIFT:
            return "ARSHIFT";
    }
    return "?";
}

const char* TreePrinter::relOpStr(RelOp op) {
    switch (op) {
        case RelOp::EQ:
            return "EQ";
        case RelOp::NE:
            return "NE";
        case RelOp::LT:
            return "LT";
        case RelOp::GT:
            return "GT";
        case RelOp::LE:
            return "LE";
        case RelOp::GE:
            return "GE";
        case RelOp::ULT:
            return "ULT";
        case RelOp::ULE:
            return "ULE";
        case RelOp::UGT:
            return "UGT";
        case RelOp::UGE:
            return "UGE";
    }
    return "?";
}

void TreePrinter::printIndent() {
    for (int i = 0; i < indent_; ++i) {
        os_ << ' ';
    }
}

void TreePrinter::print(const ExpPtr& exp) {
    if (!exp) {
        os_ << "(null)";
        return;
    }
    if (auto e = dynamic_cast<const ConstExp*>(exp.get())) {
        printExp(e);
    } else if (auto e = dynamic_cast<const NameExp*>(exp.get())) {
        printExp(e);
    } else if (auto e = dynamic_cast<const TempExp*>(exp.get())) {
        printExp(e);
    } else if (auto e = dynamic_cast<const BinOpExp*>(exp.get())) {
        printExp(e);
    } else if (auto e = dynamic_cast<const MemExp*>(exp.get())) {
        printExp(e);
    } else if (auto e = dynamic_cast<const CallExp*>(exp.get())) {
        printExp(e);
    } else if (auto e = dynamic_cast<const EseqExp*>(exp.get())) {
        printExp(e);
    } else {
        os_ << "(unknown exp)";
    }
}

void TreePrinter::print(const StmPtr& stm) {
    if (!stm) {
        printIndent();
        os_ << "(null)\n";
        return;
    }
    if (auto s = dynamic_cast<const MoveStm*>(stm.get())) {
        printStm(s);
    } else if (auto s = dynamic_cast<const ExpStm*>(stm.get())) {
        printStm(s);
    } else if (auto s = dynamic_cast<const JumpStm*>(stm.get())) {
        printStm(s);
    } else if (auto s = dynamic_cast<const CJumpStm*>(stm.get())) {
        printStm(s);
    } else if (auto s = dynamic_cast<const SeqStm*>(stm.get())) {
        printStm(s);
    } else if (auto s = dynamic_cast<const LabelStm*>(stm.get())) {
        printStm(s);
    } else {
        printIndent();
        os_ << "(unknown stm)\n";
    }
}

//==============================================================================
// Expression Printers
//==============================================================================

void TreePrinter::printExp(const ConstExp* exp) { os_ << "CONST(" << exp->value() << ")"; }

void TreePrinter::printExp(const NameExp* exp) { os_ << "NAME(" << exp->label().name() << ")"; }

void TreePrinter::printExp(const TempExp* exp) { os_ << "TEMP(" << exp->temp().toString() << ")"; }

void TreePrinter::printExp(const BinOpExp* exp) {
    os_ << "BINOP(" << binOpStr(exp->op()) << ",\n";
    incIndent();
    printIndent();
    print(exp->left());
    os_ << ",\n";
    printIndent();
    print(exp->right());
    os_ << ")";
    decIndent();
}

void TreePrinter::printExp(const MemExp* exp) {
    os_ << "MEM(\n";
    incIndent();
    printIndent();
    print(exp->addr());
    os_ << ")";
    decIndent();
}

void TreePrinter::printExp(const CallExp* exp) {
    os_ << "CALL(\n";
    incIndent();
    printIndent();
    print(exp->func());
    for (const auto& arg : exp->args()) {
        os_ << ",\n";
        printIndent();
        print(arg);
    }
    os_ << ")";
    decIndent();
}

void TreePrinter::printExp(const EseqExp* exp) {
    os_ << "ESEQ(\n";
    incIndent();
    print(exp->stm());
    printIndent();
    print(exp->exp());
    os_ << ")";
    decIndent();
}

//==============================================================================
// Statement Printers
//==============================================================================

void TreePrinter::printStm(const MoveStm* stm) {
    printIndent();
    os_ << "MOVE(\n";
    incIndent();
    printIndent();
    print(stm->dst());
    os_ << ",\n";
    printIndent();
    print(stm->src());
    os_ << ")\n";
    decIndent();
}

void TreePrinter::printStm(const ExpStm* stm) {
    printIndent();
    os_ << "EXP(\n";
    incIndent();
    printIndent();
    print(stm->exp());
    os_ << ")\n";
    decIndent();
}

void TreePrinter::printStm(const JumpStm* stm) {
    printIndent();
    os_ << "JUMP(\n";
    incIndent();
    printIndent();
    print(stm->exp());
    os_ << ", [";
    for (size_t i = 0; i < stm->targets().size(); ++i) {
        if (i > 0) os_ << ", ";
        os_ << stm->targets()[i].name();
    }
    os_ << "])\n";
    decIndent();
}

void TreePrinter::printStm(const CJumpStm* stm) {
    printIndent();
    os_ << "CJUMP(" << relOpStr(stm->op()) << ",\n";
    incIndent();
    printIndent();
    print(stm->left());
    os_ << ",\n";
    printIndent();
    print(stm->right());
    os_ << ",\n";
    printIndent();
    os_ << stm->trueLabel().name() << ", " << stm->falseLabel().name() << ")\n";
    decIndent();
}

void TreePrinter::printStm(const SeqStm* stm) {
    print(stm->first());
    print(stm->second());
}

void TreePrinter::printStm(const LabelStm* stm) {
    printIndent();
    os_ << "LABEL(" << stm->label().name() << ")\n";
}

}  // namespace ir
}  // namespace tiger
