#include "translate/translate.hpp"

namespace tiger {
namespace translate {

// Level implementation
LevelPtr Level::outermost(FrameFactory& factory) {
    // Outermost level has no parent and a frame for "main"
    auto frame = factory.newFrame(factory.tempFactory().namedLabel("_main"), {});
    return LevelPtr(new Level(nullptr, frame));
}

LevelPtr Level::newLevel(LevelPtr parent, Label name, const std::vector<bool>& formals,
                         FrameFactory& factory) {
    // Add static link as first formal (always escapes)
    std::vector<bool> allFormals;
    allFormals.push_back(true);  // static link escapes
    allFormals.insert(allFormals.end(), formals.begin(), formals.end());

    auto frame = factory.newFrame(name, allFormals);
    return LevelPtr(new Level(parent, frame));
}

std::vector<AccessPtr> Level::formals() const {
    // Return all formals except the static link (first one)
    const auto& allFormals = frame_->formals();
    if (allFormals.size() <= 1) {
        return {};
    }
    return std::vector<AccessPtr>(allFormals.begin() + 1, allFormals.end());
}

// Translator implementation
Translator::Translator(FrameFactoryPtr factory) : factory_(factory) {
    outermost_ = Level::outermost(*factory_);
    currentLevel_ = outermost_;
}

void Translator::enterFunction(Label name, const std::vector<bool>& formals) {
    currentLevel_ = Level::newLevel(currentLevel_, name, formals, *factory_);
}

void Translator::exitFunction() {
    if (currentLevel_ && currentLevel_->parent()) {
        currentLevel_ = currentLevel_->parent();
    }
}

VarAccess Translator::allocLocal(bool escape) {
    auto access = currentLevel_->frame()->allocLocal(escape);
    return VarAccess(currentLevel_, access);
}

std::vector<VarAccess> Translator::formals() const {
    std::vector<VarAccess> result;
    for (auto& access : currentLevel_->formals()) {
        result.emplace_back(currentLevel_, access);
    }
    return result;
}

}  // namespace translate
}  // namespace tiger
