#ifndef TIGER_TRANSLATE_MIPS_FRAME_HPP
#define TIGER_TRANSLATE_MIPS_FRAME_HPP

#include "translate/frame.hpp"

namespace tiger {
namespace translate {

/**
 * MipsFrame - MIPS32 specific frame implementation
 *
 * MIPS calling convention (O32 ABI):
 * - First 4 integer args in: $a0-$a3 ($4-$7)
 * - Return value in $v0 ($2), optionally $v1 ($3)
 * - Callee-saved: $s0-$s7 ($16-$23), $fp ($30), $ra ($31)
 * - Caller-saved: $t0-$t9 ($8-$15, $24-$25), $a0-$a3, $v0-$v1
 * - Frame pointer: $fp ($30)
 * - Stack pointer: $sp ($29)
 * - Return address: $ra ($31)
 *
 * Frame layout (growing downward, stack grows toward lower addresses):
 *   [higher addresses]
 *   +------------------+
 *   | argument N       |  (if > 4 args)
 *   | ...              |
 *   | argument 5       |  fp + 16 + (n-5)*4
 *   +------------------+
 *   | argument 4 space |  fp + 16  (reserved even if passed in $a3)
 *   | argument 3 space |  fp + 12  (reserved even if passed in $a2)
 *   | argument 2 space |  fp + 8   (reserved even if passed in $a1)
 *   | argument 1 space |  fp + 4   (reserved even if passed in $a0)
 *   +------------------+
 *   | saved $ra        |  fp + 0   (return address)
 *   +------------------+
 *   | saved $fp        |  <- fp (frame pointer points here)
 *   +------------------+
 *   | local var 1      |  fp - 4
 *   | local var 2      |  fp - 8
 *   | ...              |
 *   +------------------+
 *   [lower addresses]   <- sp
 *
 * Note: MIPS O32 ABI requires 4 words of argument space to always be
 * reserved on the stack, even for functions with <= 4 arguments.
 */
class MipsFrame : public Frame {
public:
    MipsFrame(Label name, const std::vector<bool>& formals, TempFactory& tempFactory);

    Label name() const override { return name_; }
    const std::vector<AccessPtr>& formals() const override { return formals_; }
    AccessPtr allocLocal(bool escape) override;
    Temp framePointer() const override { return fp_; }
    Temp returnValue() const override { return rv_; }
    int wordSize() const override { return WORD_SIZE; }
    std::string toString() const override;

    // MIPS specific registers
    Temp stackPointer() const { return sp_; }
    Temp returnAddress() const { return ra_; }

    static constexpr int WORD_SIZE = 4;
    static constexpr int MAX_REG_ARGS = 4;
    static constexpr int ARG_SPACE = 16;  // Reserved space for $a0-$a3

private:
    Label name_;
    std::vector<AccessPtr> formals_;
    int localOffset_;  // Next available local offset (negative from fp)
    TempFactory& tempFactory_;
    Temp fp_;  // Frame pointer ($fp/$30)
    Temp sp_;  // Stack pointer ($sp/$29)
    Temp rv_;  // Return value ($v0/$2)
    Temp ra_;  // Return address ($ra/$31)
};

/**
 * MipsFrameFactory - Factory for creating MIPS frames
 */
class MipsFrameFactory : public FrameFactory {
public:
    MipsFrameFactory() = default;

    FramePtr newFrame(Label name, const std::vector<bool>& formals) override {
        return std::make_shared<MipsFrame>(name, formals, tempFactory_);
    }

    TempFactory& tempFactory() override { return tempFactory_; }

private:
    TempFactory tempFactory_;
};

}  // namespace translate
}  // namespace tiger

#endif  // TIGER_TRANSLATE_MIPS_FRAME_HPP
