#ifndef TIGER_IR_TREE_HPP
#define TIGER_IR_TREE_HPP

#include <memory>
#include <string>
#include <vector>

#include "translate/temp.hpp"

namespace tiger {
namespace ir {

using translate::Label;
using translate::Temp;

// Forward declarations
class Exp;
class Stm;

using ExpPtr = std::shared_ptr<Exp>;
using StmPtr = std::shared_ptr<Stm>;
using ExpList = std::vector<ExpPtr>;
using StmList = std::vector<StmPtr>;

/**
 * Binary operators
 */
enum class BinOp {
    PLUS,
    MINUS,
    MUL,
    DIV,  // Arithmetic
    AND,
    OR,
    XOR,  // Bitwise
    LSHIFT,
    RSHIFT,
    ARSHIFT  // Shifts
};

/**
 * Relational operators (for CJUMP)
 */
enum class RelOp {
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE,  // Signed
    ULT,
    ULE,
    UGT,
    UGE  // Unsigned
};

// Get negated relational operator
inline RelOp notRel(RelOp op) {
    switch (op) {
        case RelOp::EQ:
            return RelOp::NE;
        case RelOp::NE:
            return RelOp::EQ;
        case RelOp::LT:
            return RelOp::GE;
        case RelOp::GE:
            return RelOp::LT;
        case RelOp::GT:
            return RelOp::LE;
        case RelOp::LE:
            return RelOp::GT;
        case RelOp::ULT:
            return RelOp::UGE;
        case RelOp::UGE:
            return RelOp::ULT;
        case RelOp::UGT:
            return RelOp::ULE;
        case RelOp::ULE:
            return RelOp::UGT;
    }
    return op;
}

// Swap operands and adjust relational operator
inline RelOp commute(RelOp op) {
    switch (op) {
        case RelOp::EQ:
            return RelOp::EQ;
        case RelOp::NE:
            return RelOp::NE;
        case RelOp::LT:
            return RelOp::GT;
        case RelOp::GT:
            return RelOp::LT;
        case RelOp::LE:
            return RelOp::GE;
        case RelOp::GE:
            return RelOp::LE;
        case RelOp::ULT:
            return RelOp::UGT;
        case RelOp::UGT:
            return RelOp::ULT;
        case RelOp::ULE:
            return RelOp::UGE;
        case RelOp::UGE:
            return RelOp::ULE;
    }
    return op;
}

//==============================================================================
// IR Expressions (produce a value)
//==============================================================================

/**
 * Exp - Base class for all IR expressions
 */
class Exp {
public:
    virtual ~Exp() = default;
    virtual std::string toString() const = 0;
};

/**
 * CONST - Integer constant
 */
class ConstExp : public Exp {
public:
    explicit ConstExp(int value) : value_(value) {}
    int value() const { return value_; }
    std::string toString() const override { return "CONST(" + std::to_string(value_) + ")"; }

private:
    int value_;
};

/**
 * NAME - Symbolic constant (assembly language label)
 */
class NameExp : public Exp {
public:
    explicit NameExp(const Label& label) : label_(label) {}
    const Label& label() const { return label_; }
    std::string toString() const override { return "NAME(" + label_.name() + ")"; }

private:
    Label label_;
};

/**
 * TEMP - Temporary (abstract register)
 */
class TempExp : public Exp {
public:
    explicit TempExp(const Temp& temp) : temp_(temp) {}
    const Temp& temp() const { return temp_; }
    std::string toString() const override { return "TEMP(" + temp_.toString() + ")"; }

private:
    Temp temp_;
};

/**
 * BINOP - Binary operation
 */
class BinOpExp : public Exp {
public:
    BinOpExp(BinOp op, ExpPtr left, ExpPtr right)
        : op_(op), left_(std::move(left)), right_(std::move(right)) {}
    BinOp op() const { return op_; }
    const ExpPtr& left() const { return left_; }
    const ExpPtr& right() const { return right_; }
    std::string toString() const override;

private:
    BinOp op_;
    ExpPtr left_;
    ExpPtr right_;
};

/**
 * MEM - Memory access (contents of a word of memory starting at address)
 */
class MemExp : public Exp {
public:
    explicit MemExp(ExpPtr addr) : addr_(std::move(addr)) {}
    const ExpPtr& addr() const { return addr_; }
    std::string toString() const override { return "MEM(" + addr_->toString() + ")"; }

private:
    ExpPtr addr_;
};

/**
 * CALL - Function call
 */
class CallExp : public Exp {
public:
    CallExp(ExpPtr func, ExpList args) : func_(std::move(func)), args_(std::move(args)) {}
    const ExpPtr& func() const { return func_; }
    const ExpList& args() const { return args_; }
    std::string toString() const override;

private:
    ExpPtr func_;
    ExpList args_;
};

/**
 * ESEQ - Expression sequence (evaluate stm for side effects, return exp)
 */
class EseqExp : public Exp {
public:
    EseqExp(StmPtr stm, ExpPtr exp) : stm_(std::move(stm)), exp_(std::move(exp)) {}
    const StmPtr& stm() const { return stm_; }
    const ExpPtr& exp() const { return exp_; }
    std::string toString() const override;

private:
    StmPtr stm_;
    ExpPtr exp_;
};

//==============================================================================
// IR Statements (perform side effects, produce no value)
//==============================================================================

/**
 * Stm - Base class for all IR statements
 */
class Stm {
public:
    virtual ~Stm() = default;
    virtual std::string toString() const = 0;
};

/**
 * MOVE - Move source expression to destination
 *        MOVE(TEMP t, e) - store e into temporary t
 *        MOVE(MEM(e1), e2) - store e2 at memory address e1
 */
class MoveStm : public Stm {
public:
    MoveStm(ExpPtr dst, ExpPtr src) : dst_(std::move(dst)), src_(std::move(src)) {}
    const ExpPtr& dst() const { return dst_; }
    const ExpPtr& src() const { return src_; }
    std::string toString() const override {
        return "MOVE(" + dst_->toString() + ", " + src_->toString() + ")";
    }

private:
    ExpPtr dst_;
    ExpPtr src_;
};

/**
 * EXP - Evaluate expression and discard result (for side effects)
 */
class ExpStm : public Stm {
public:
    explicit ExpStm(ExpPtr exp) : exp_(std::move(exp)) {}
    const ExpPtr& exp() const { return exp_; }
    std::string toString() const override { return "EXP(" + exp_->toString() + ")"; }

private:
    ExpPtr exp_;
};

/**
 * JUMP - Jump to address (usually NAME(label))
 *        targets is list of possible destination labels (for dataflow analysis)
 */
class JumpStm : public Stm {
public:
    JumpStm(ExpPtr exp, std::vector<Label> targets)
        : exp_(std::move(exp)), targets_(std::move(targets)) {}
    const ExpPtr& exp() const { return exp_; }
    const std::vector<Label>& targets() const { return targets_; }
    std::string toString() const override;

private:
    ExpPtr exp_;
    std::vector<Label> targets_;
};

/**
 * CJUMP - Conditional jump
 *         If left op right is true, jump to trueLabel; else jump to falseLabel
 */
class CJumpStm : public Stm {
public:
    CJumpStm(RelOp op, ExpPtr left, ExpPtr right, Label trueLabel, Label falseLabel)
        : op_(op),
          left_(std::move(left)),
          right_(std::move(right)),
          trueLabel_(trueLabel),
          falseLabel_(falseLabel) {}
    RelOp op() const { return op_; }
    const ExpPtr& left() const { return left_; }
    const ExpPtr& right() const { return right_; }
    const Label& trueLabel() const { return trueLabel_; }
    const Label& falseLabel() const { return falseLabel_; }
    std::string toString() const override;

private:
    RelOp op_;
    ExpPtr left_;
    ExpPtr right_;
    Label trueLabel_;
    Label falseLabel_;
};

/**
 * SEQ - Statement sequence (execute first then second)
 */
class SeqStm : public Stm {
public:
    SeqStm(StmPtr first, StmPtr second) : first_(std::move(first)), second_(std::move(second)) {}
    const StmPtr& first() const { return first_; }
    const StmPtr& second() const { return second_; }
    std::string toString() const override {
        return "SEQ(" + first_->toString() + ", " + second_->toString() + ")";
    }

private:
    StmPtr first_;
    StmPtr second_;
};

/**
 * LABEL - Define a label at this point
 */
class LabelStm : public Stm {
public:
    explicit LabelStm(const Label& label) : label_(label) {}
    const Label& label() const { return label_; }
    std::string toString() const override { return "LABEL(" + label_.name() + ")"; }

private:
    Label label_;
};

//==============================================================================
// Factory functions for creating IR nodes
//==============================================================================

inline ExpPtr CONST(int value) { return std::make_shared<ConstExp>(value); }
inline ExpPtr NAME(const Label& l) { return std::make_shared<NameExp>(l); }
inline ExpPtr TEMP(const Temp& t) { return std::make_shared<TempExp>(t); }
inline ExpPtr BINOP(BinOp op, ExpPtr l, ExpPtr r) {
    return std::make_shared<BinOpExp>(op, std::move(l), std::move(r));
}
inline ExpPtr MEM(ExpPtr addr) { return std::make_shared<MemExp>(std::move(addr)); }
inline ExpPtr CALL(ExpPtr func, ExpList args) {
    return std::make_shared<CallExp>(std::move(func), std::move(args));
}
inline ExpPtr ESEQ(StmPtr s, ExpPtr e) {
    return std::make_shared<EseqExp>(std::move(s), std::move(e));
}

inline StmPtr MOVE(ExpPtr dst, ExpPtr src) {
    return std::make_shared<MoveStm>(std::move(dst), std::move(src));
}
inline StmPtr EXP(ExpPtr e) { return std::make_shared<ExpStm>(std::move(e)); }
inline StmPtr JUMP(const Label& l) {
    return std::make_shared<JumpStm>(NAME(l), std::vector<Label>{l});
}
inline StmPtr JUMP(ExpPtr e, std::vector<Label> targets) {
    return std::make_shared<JumpStm>(std::move(e), std::move(targets));
}
inline StmPtr CJUMP(RelOp op, ExpPtr l, ExpPtr r, Label t, Label f) {
    return std::make_shared<CJumpStm>(op, std::move(l), std::move(r), t, f);
}
inline StmPtr SEQ(StmPtr s1, StmPtr s2) {
    if (!s1) return s2;
    if (!s2) return s1;
    return std::make_shared<SeqStm>(std::move(s1), std::move(s2));
}
inline StmPtr SEQ(std::initializer_list<StmPtr> stms) {
    StmPtr result = nullptr;
    for (auto& s : stms) {
        result = SEQ(std::move(result), s);
    }
    return result;
}
inline StmPtr LABEL(const Label& l) { return std::make_shared<LabelStm>(l); }

}  // namespace ir
}  // namespace tiger

#endif  // TIGER_IR_TREE_HPP
