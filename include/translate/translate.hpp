#ifndef TIGER_TRANSLATE_TRANSLATE_HPP
#define TIGER_TRANSLATE_TRANSLATE_HPP

#include <memory>
#include <vector>

#include "translate/frame.hpp"
#include "translate/temp.hpp"

namespace tiger {
namespace translate {

/**
 * Level - Represents a function nesting level
 *
 * Each function has a Level that contains:
 * - A link to the enclosing level (for static link)
 * - The frame for this function
 *
 * The outermost level represents the "main" program.
 */
class Level;
using LevelPtr = std::shared_ptr<Level>;

class Level {
public:
    // Create the outermost level (main program)
    static LevelPtr outermost(FrameFactory& factory);

    // Create a new nested level
    static LevelPtr newLevel(LevelPtr parent, Label name, const std::vector<bool>& formals,
                             FrameFactory& factory);

    LevelPtr parent() const { return parent_; }
    FramePtr frame() const { return frame_; }

    // Get formals, excluding the static link
    std::vector<AccessPtr> formals() const;

private:
    Level(LevelPtr parent, FramePtr frame) : parent_(parent), frame_(frame) {}

    LevelPtr parent_;
    FramePtr frame_;
};

/**
 * VarAccess - Combines Level and Access for variable lookup
 *
 * When looking up a variable, we need both:
 * - The level where it was declared
 * - The access (InFrame or InReg) within that level
 */
struct VarAccess {
    LevelPtr level;
    AccessPtr access;

    VarAccess() = default;
    VarAccess(LevelPtr l, AccessPtr a) : level(l), access(a) {}
};

/**
 * Translator - Bridge between semantic analysis and IR generation
 *
 * This class manages:
 * - Function nesting levels
 * - Variable allocation with escape info
 * - Static link chain traversal
 */
class Translator {
public:
    explicit Translator(FrameFactoryPtr factory);

    // Level management
    LevelPtr outermost() const { return outermost_; }
    LevelPtr currentLevel() const { return currentLevel_; }

    // Enter/exit a function
    void enterFunction(Label name, const std::vector<bool>& formals);
    void exitFunction();

    // Allocate a local variable
    VarAccess allocLocal(bool escape);

    // Get function formals (excluding static link)
    std::vector<VarAccess> formals() const;

    // Get temp factory
    TempFactory& tempFactory() { return factory_->tempFactory(); }

private:
    FrameFactoryPtr factory_;
    LevelPtr outermost_;
    LevelPtr currentLevel_;
};

}  // namespace translate
}  // namespace tiger

#endif  // TIGER_TRANSLATE_TRANSLATE_HPP
