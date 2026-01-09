#ifndef TIGER_IR_IR_GENERATOR_HPP
#define TIGER_IR_IR_GENERATOR_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "ast/ast.hpp"
#include "ir/translate_exp.hpp"
#include "ir/tree.hpp"
#include "semantic/types.hpp"
#include "translate/frame.hpp"
#include "translate/translate.hpp"

namespace tiger {
namespace ir {

/**
 * Fragment - Result of translating a function or string
 */
class Fragment {
public:
    virtual ~Fragment() = default;
};

/**
 * ProcFragment - A function body
 */
class ProcFragment : public Fragment {
public:
    ProcFragment(StmPtr body, translate::FramePtr frame)
        : body_(std::move(body)), frame_(std::move(frame)) {}

    const StmPtr& body() const { return body_; }
    const translate::FramePtr& frame() const { return frame_; }

private:
    StmPtr body_;
    translate::FramePtr frame_;
};

/**
 * StringFragment - A string literal
 */
class StringFragment : public Fragment {
public:
    StringFragment(Label label, const std::string& value) : label_(label), value_(value) {}

    const Label& label() const { return label_; }
    const std::string& value() const { return value_; }

private:
    Label label_;
    std::string value_;
};

using FragmentPtr = std::shared_ptr<Fragment>;
using FragmentList = std::vector<FragmentPtr>;

/**
 * VarEntry - Information about a variable for IR generation
 */
struct IRVarEntry {
    translate::LevelPtr level;
    translate::AccessPtr access;

    IRVarEntry() = default;
    IRVarEntry(translate::LevelPtr l, translate::AccessPtr a)
        : level(std::move(l)), access(std::move(a)) {}
};

/**
 * FunEntry - Information about a function for IR generation
 */
struct IRFunEntry {
    translate::LevelPtr level;
    Label label;

    IRFunEntry() = default;
    IRFunEntry(translate::LevelPtr l, Label lab) : level(std::move(l)), label(lab) {}
};

/**
 * IRGenerator - Generates IR from AST
 *
 * This visitor traverses the AST and produces IR Tree nodes.
 * It uses the semantic analysis type information and translate module
 * for frame management.
 */
class IRGenerator : public ast::Visitor<TransExpPtr> {
public:
    IRGenerator(translate::FrameFactoryPtr frameFactory);
    ~IRGenerator();

    // Generate IR for the entire program
    void generate(ast::Expr* program);

    // Get the generated fragments
    const FragmentList& fragments() const { return fragments_; }

    // Expression visitors
    TransExpPtr visit(ast::VarExpr* expr) override;
    TransExpPtr visit(ast::NilExpr* expr) override;
    TransExpPtr visit(ast::IntExpr* expr) override;
    TransExpPtr visit(ast::StringExpr* expr) override;
    TransExpPtr visit(ast::CallExpr* expr) override;
    TransExpPtr visit(ast::OpExpr* expr) override;
    TransExpPtr visit(ast::RecordExpr* expr) override;
    TransExpPtr visit(ast::ArrayExpr* expr) override;
    TransExpPtr visit(ast::AssignExpr* expr) override;
    TransExpPtr visit(ast::IfExpr* expr) override;
    TransExpPtr visit(ast::WhileExpr* expr) override;
    TransExpPtr visit(ast::ForExpr* expr) override;
    TransExpPtr visit(ast::BreakExpr* expr) override;
    TransExpPtr visit(ast::LetExpr* expr) override;
    TransExpPtr visit(ast::SeqExpr* expr) override;

    // Declaration visitors (return Nx or nullptr)
    TransExpPtr visit(ast::TypeDecl* decl) override;
    TransExpPtr visit(ast::VarDecl* decl) override;
    TransExpPtr visit(ast::FunctionDecl* decl) override;

    // Type visitors (no IR generated)
    TransExpPtr visit(ast::NameType*) override { return nullptr; }
    TransExpPtr visit(ast::RecordType*) override { return nullptr; }
    TransExpPtr visit(ast::ArrayType*) override { return nullptr; }

private:
    translate::FrameFactoryPtr frameFactory_;
    translate::TempFactory tempFactory_;
    translate::LevelPtr currentLevel_;
    FragmentList fragments_;

    // Break label stack for while/for loops
    std::vector<Label> breakLabels_;

    // Variable and function environments
    std::unordered_map<std::string, IRVarEntry> varEnv_;
    std::unordered_map<std::string, IRFunEntry> funEnv_;

    // Scope management
    std::vector<std::vector<std::string>> varScopes_;
    std::vector<std::vector<std::string>> funScopes_;

    void beginScope();
    void endScope();

    // Add to current scope
    void addVar(const std::string& name, const IRVarEntry& entry);
    void addFun(const std::string& name, const IRFunEntry& entry);

    // Lookup
    IRVarEntry* lookupVar(const std::string& name);
    IRFunEntry* lookupFun(const std::string& name);

    // Helper to create static link chain access
    ExpPtr staticLinkChain(translate::LevelPtr fromLevel, translate::LevelPtr toLevel);

    // Helper to access variable at given level
    ExpPtr accessVar(const IRVarEntry& var);

    // String literal handling
    Label stringLiteral(const std::string& s);

    // Add fragment
    void addFragment(FragmentPtr frag);

    // Create procedure entry/exit
    StmPtr procEntryExit(StmPtr body);

    // Initialize built-in functions
    void initBuiltins();

    // Word size helper
    int wordSize() const;
};

}  // namespace ir
}  // namespace tiger

#endif  // TIGER_IR_IR_GENERATOR_HPP
