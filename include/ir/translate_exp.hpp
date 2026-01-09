#ifndef TIGER_IR_TRANSLATE_EXP_HPP
#define TIGER_IR_TRANSLATE_EXP_HPP

#include <functional>
#include <memory>

#include "ir/tree.hpp"

namespace tiger {
namespace ir {

/**
 * Translate Expressions - Wrapper types for IR generation
 *
 * Three kinds of expressions:
 * - Ex: Expression that computes a value
 * - Nx: Expression that computes no value (statement)
 * - Cx: Conditional expression (produces true/false result via jumps)
 */

class TransExp;
using TransExpPtr = std::shared_ptr<TransExp>;

// Type for conditional: given true/false labels, produce statement
using CondFn = std::function<StmPtr(Label trueL, Label falseL)>;

class TransExp {
public:
    virtual ~TransExp() = default;

    // Convert to expression (returns a value)
    virtual ExpPtr unEx(translate::TempFactory& temps) = 0;

    // Convert to statement (for side effects only)
    virtual StmPtr unNx(translate::TempFactory& temps) = 0;

    // Convert to conditional (jumps to true/false labels)
    virtual StmPtr unCx(Label trueL, Label falseL, translate::TempFactory& temps) = 0;
};

/**
 * Ex - Expression wrapper
 * Wraps an IR expression that computes a value
 */
class Ex : public TransExp {
public:
    explicit Ex(ExpPtr exp) : exp_(std::move(exp)) {}

    ExpPtr unEx(translate::TempFactory&) override { return exp_; }

    StmPtr unNx(translate::TempFactory&) override { return EXP(exp_); }

    StmPtr unCx(Label trueL, Label falseL, translate::TempFactory&) override {
        // Compare expression != 0
        return CJUMP(RelOp::NE, exp_, CONST(0), trueL, falseL);
    }

private:
    ExpPtr exp_;
};

/**
 * Nx - No-result expression wrapper
 * Wraps an IR statement (no value)
 */
class Nx : public TransExp {
public:
    explicit Nx(StmPtr stm) : stm_(std::move(stm)) {}

    ExpPtr unEx(translate::TempFactory&) override {
        // Error: Nx has no value
        // Return 0 as a fallback
        return CONST(0);
    }

    StmPtr unNx(translate::TempFactory&) override { return stm_; }

    StmPtr unCx(Label trueL, Label falseL, translate::TempFactory&) override {
        // Error: Nx cannot be used as condition
        // Jump to false as fallback
        return JUMP(falseL);
    }

private:
    StmPtr stm_;
};

/**
 * Cx - Conditional expression wrapper
 * Represents a conditional that jumps to true/false labels
 */
class Cx : public TransExp {
public:
    explicit Cx(CondFn condFn) : condFn_(std::move(condFn)) {}

    ExpPtr unEx(translate::TempFactory& temps) override {
        // Convert condition to expression:
        // let r = 1 in (if cond then () else r := 0); r
        Temp r = temps.newTemp();
        Label t = temps.newLabel();
        Label f = temps.newLabel();
        Label join = temps.newLabel();

        return ESEQ(SEQ({MOVE(TEMP(r), CONST(1)), condFn_(t, f), LABEL(f), MOVE(TEMP(r), CONST(0)),
                         JUMP(join), LABEL(t), JUMP(join), LABEL(join)}),
                    TEMP(r));
    }

    StmPtr unNx(translate::TempFactory& temps) override {
        // Evaluate condition for side effects, discard result
        Label t = temps.newLabel();
        Label f = temps.newLabel();
        return SEQ({condFn_(t, f), LABEL(t), LABEL(f)});
    }

    StmPtr unCx(Label trueL, Label falseL, translate::TempFactory&) override {
        return condFn_(trueL, falseL);
    }

private:
    CondFn condFn_;
};

// Factory functions
inline TransExpPtr makeEx(ExpPtr exp) { return std::make_shared<Ex>(std::move(exp)); }
inline TransExpPtr makeNx(StmPtr stm) { return std::make_shared<Nx>(std::move(stm)); }
inline TransExpPtr makeCx(CondFn fn) { return std::make_shared<Cx>(std::move(fn)); }

// Convenience: make Ex from constant
inline TransExpPtr makeConst(int value) { return makeEx(CONST(value)); }

}  // namespace ir
}  // namespace tiger

#endif  // TIGER_IR_TRANSLATE_EXP_HPP
