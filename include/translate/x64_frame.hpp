#ifndef TIGER_TRANSLATE_X64_FRAME_HPP
#define TIGER_TRANSLATE_X64_FRAME_HPP

#include "translate/frame.hpp"

namespace tiger {
namespace translate {

/**
 * X64Frame - x86-64 specific frame implementation
 *
 * x86-64 calling convention (System V AMD64 ABI):
 * - First 6 integer args in: RDI, RSI, RDX, RCX, R8, R9
 * - Return value in RAX
 * - Callee-saved: RBX, RBP, R12-R15
 * - Caller-saved: RAX, RCX, RDX, RSI, RDI, R8-R11
 *
 * Frame layout (growing downward):
 *   [higher addresses]
 *   +------------------+
 *   | argument N       |  (if > 6 args)
 *   | ...              |
 *   | argument 7       |  rbp + 16 + (n-7)*8
 *   +------------------+
 *   | return address   |  (pushed by call)
 *   +------------------+
 *   | saved rbp        |  <- rbp
 *   +------------------+
 *   | local var 1      |  rbp - 8
 *   | local var 2      |  rbp - 16
 *   | ...              |
 *   +------------------+
 *   [lower addresses]   <- rsp
 */
class X64Frame : public Frame {
public:
    X64Frame(Label name, const std::vector<bool>& formals, TempFactory& tempFactory);

    Label name() const override { return name_; }
    const std::vector<AccessPtr>& formals() const override { return formals_; }
    AccessPtr allocLocal(bool escape) override;
    Temp framePointer() const override { return fp_; }
    Temp returnValue() const override { return rv_; }
    int wordSize() const override { return WORD_SIZE; }
    std::string toString() const override;

    static constexpr int WORD_SIZE = 8;
    static constexpr int MAX_REG_ARGS = 6;

private:
    Label name_;
    std::vector<AccessPtr> formals_;
    int localOffset_;  // Next available local offset (negative)
    TempFactory& tempFactory_;
    Temp fp_;  // Frame pointer temp
    Temp rv_;  // Return value temp
};

/**
 * X64FrameFactory - Factory for creating X64 frames
 */
class X64FrameFactory : public FrameFactory {
public:
    X64FrameFactory() = default;

    FramePtr newFrame(Label name, const std::vector<bool>& formals) override {
        return std::make_shared<X64Frame>(name, formals, tempFactory_);
    }

    TempFactory& tempFactory() override { return tempFactory_; }

private:
    TempFactory tempFactory_;
};

}  // namespace translate
}  // namespace tiger

#endif  // TIGER_TRANSLATE_X64_FRAME_HPP
