#include "translate/x64_frame.hpp"

#include <sstream>

namespace tiger {
namespace translate {

X64Frame::X64Frame(Label name, const std::vector<bool>& formals, TempFactory& tempFactory)
    : name_(name),
      localOffset_(0),
      tempFactory_(tempFactory),
      fp_(tempFactory.newTemp()),
      rv_(tempFactory.newTemp()) {
    // Process each formal parameter
    for (size_t i = 0; i < formals.size(); ++i) {
        bool escape = formals[i];

        if (escape || i >= MAX_REG_ARGS) {
            // Escaping or overflow args go to stack
            // For escaped register args, they need to be saved to stack
            // Overflow args are already on stack
            if (i < MAX_REG_ARGS) {
                // Register arg that escapes - allocate stack space
                localOffset_ -= WORD_SIZE;
                formals_.push_back(std::make_shared<InFrame>(localOffset_));
            } else {
                // Stack arg (arg index i >= 6)
                // Located at rbp + 16 + (i - MAX_REG_ARGS) * WORD_SIZE
                int offset = 16 + (static_cast<int>(i) - MAX_REG_ARGS) * WORD_SIZE;
                formals_.push_back(std::make_shared<InFrame>(offset));
            }
        } else {
            // Non-escaping arg in register
            formals_.push_back(std::make_shared<InReg>(tempFactory_.newTemp()));
        }
    }
}

AccessPtr X64Frame::allocLocal(bool escape) {
    if (escape) {
        localOffset_ -= WORD_SIZE;
        return std::make_shared<InFrame>(localOffset_);
    } else {
        return std::make_shared<InReg>(tempFactory_.newTemp());
    }
}

std::string X64Frame::toString() const {
    std::ostringstream ss;
    ss << "X64Frame(" << name_.toString() << ") {\n";
    ss << "  formals: [";
    for (size_t i = 0; i < formals_.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << formals_[i]->toString();
    }
    ss << "]\n";
    ss << "  localOffset: " << localOffset_ << "\n";
    ss << "}";
    return ss.str();
}

}  // namespace translate
}  // namespace tiger
