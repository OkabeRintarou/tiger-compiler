#include "ir/tree.hpp"

namespace tiger {
namespace ir {

//==============================================================================
// BinOp to string
//==============================================================================
static const char* binOpToString(BinOp op) {
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
    return "UNKNOWN";
}

//==============================================================================
// RelOp to string
//==============================================================================
static const char* relOpToString(RelOp op) {
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
    return "UNKNOWN";
}

//==============================================================================
// Expression toString implementations
//==============================================================================

std::string BinOpExp::toString() const {
    return std::string("BINOP(") + binOpToString(op_) + ", " + left_->toString() + ", " +
           right_->toString() + ")";
}

std::string CallExp::toString() const {
    std::string result = "CALL(" + func_->toString();
    for (const auto& arg : args_) {
        result += ", " + arg->toString();
    }
    result += ")";
    return result;
}

std::string EseqExp::toString() const {
    return "ESEQ(" + stm_->toString() + ", " + exp_->toString() + ")";
}

//==============================================================================
// Statement toString implementations
//==============================================================================

std::string JumpStm::toString() const {
    std::string result = "JUMP(" + exp_->toString() + ", [";
    for (size_t i = 0; i < targets_.size(); ++i) {
        if (i > 0) result += ", ";
        result += targets_[i].name();
    }
    result += "])";
    return result;
}

std::string CJumpStm::toString() const {
    return std::string("CJUMP(") + relOpToString(op_) + ", " + left_->toString() + ", " +
           right_->toString() + ", " + trueLabel_.name() + ", " + falseLabel_.name() + ")";
}

}  // namespace ir
}  // namespace tiger
