#ifndef TIGER_TRANSLATE_FRAME_HPP
#define TIGER_TRANSLATE_FRAME_HPP

#include <memory>
#include <string>
#include <vector>

#include "translate/temp.hpp"

namespace tiger {
namespace translate {

class Frame;
class Access;
using FramePtr = std::shared_ptr<Frame>;
using AccessPtr = std::shared_ptr<Access>;

/**
 * Access - Abstract description of where a variable/formal resides
 * Variables can be InFrame (stack) or InReg (register)
 */
class Access {
public:
    enum Kind { IN_FRAME, IN_REG };
    virtual ~Access() = default;
    virtual Kind kind() const = 0;
    virtual std::string toString() const = 0;
    bool isInFrame() const { return kind() == IN_FRAME; }
    bool isInReg() const { return kind() == IN_REG; }
};

/**
 * InFrame - Variable stored in stack frame at offset from FP
 */
class InFrame : public Access {
public:
    explicit InFrame(int offset) : offset_(offset) {}
    Kind kind() const override { return IN_FRAME; }
    int offset() const { return offset_; }
    std::string toString() const override { return "InFrame(" + std::to_string(offset_) + ")"; }

private:
    int offset_;
};

/**
 * InReg - Variable stored in a register
 */
class InReg : public Access {
public:
    explicit InReg(Temp temp) : temp_(temp) {}
    Kind kind() const override { return IN_REG; }
    Temp temp() const { return temp_; }
    std::string toString() const override { return "InReg(" + temp_.toString() + ")"; }

private:
    Temp temp_;
};

/**
 * Frame - Machine-independent interface for activation records
 */
class Frame {
public:
    virtual ~Frame() = default;

    // Frame identification
    virtual Label name() const = 0;

    // Access to formals (parameters), first is static link
    virtual const std::vector<AccessPtr>& formals() const = 0;

    // Allocate space for a local variable
    virtual AccessPtr allocLocal(bool escape) = 0;

    // Get special registers
    virtual Temp framePointer() const = 0;
    virtual Temp returnValue() const = 0;

    // Architecture word size
    virtual int wordSize() const = 0;

    virtual std::string toString() const = 0;

    // Static link is first formal
    AccessPtr staticLink() const {
        const auto& f = formals();
        return f.empty() ? nullptr : f[0];
    }
};

/**
 * FrameFactory - Abstract factory for creating frames
 */
class FrameFactory {
public:
    virtual ~FrameFactory() = default;
    virtual FramePtr newFrame(Label name, const std::vector<bool>& formals) = 0;
    virtual TempFactory& tempFactory() = 0;
};

using FrameFactoryPtr = std::shared_ptr<FrameFactory>;

}  // namespace translate
}  // namespace tiger

#endif  // TIGER_TRANSLATE_FRAME_HPP
