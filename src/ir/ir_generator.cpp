#include "ir/ir_generator.hpp"

namespace tiger {
namespace ir {

// Helper to convert Access to IR expression
static ExpPtr accessToExp(translate::Access* access, ExpPtr framePtr, int wordSize) {
    if (auto inFrame = dynamic_cast<translate::InFrame*>(access)) {
        return MEM(BINOP(BinOp::PLUS, framePtr, CONST(inFrame->offset())));
    } else if (auto inReg = dynamic_cast<translate::InReg*>(access)) {
        return TEMP(inReg->temp());
    }
    return CONST(0);
}

IRGenerator::IRGenerator(translate::FrameFactoryPtr frameFactory)
    : frameFactory_(std::move(frameFactory)) {
    // Create outermost level (main program)
    currentLevel_ = translate::Level::outermost(*frameFactory_);

    // Initialize scope stack before adding builtins
    beginScope();
    initBuiltins();
}

IRGenerator::~IRGenerator() {
    // Clean up the builtin scope created in constructor
    if (!varScopes_.empty()) {
        endScope();
    }
}

void IRGenerator::generate(ast::Expr* program) {
    beginScope();
    auto result = program->accept(*this);

    // Create the main procedure fragment
    if (result) {
        StmPtr body = result->unNx(tempFactory_);
        addFragment(std::make_shared<ProcFragment>(procEntryExit(body), currentLevel_->frame()));
    }

    endScope();
}

int IRGenerator::wordSize() const { return currentLevel_->frame()->wordSize(); }

//==============================================================================
// Scope Management
//==============================================================================

void IRGenerator::beginScope() {
    varScopes_.push_back({});
    funScopes_.push_back({});
}

void IRGenerator::endScope() {
    // Remove variables from this scope
    for (const auto& name : varScopes_.back()) {
        varEnv_.erase(name);
    }
    varScopes_.pop_back();

    // Remove functions from this scope
    for (const auto& name : funScopes_.back()) {
        funEnv_.erase(name);
    }
    funScopes_.pop_back();
}

void IRGenerator::addVar(const std::string& name, const IRVarEntry& entry) {
    varEnv_[name] = entry;
    varScopes_.back().push_back(name);
}

void IRGenerator::addFun(const std::string& name, const IRFunEntry& entry) {
    funEnv_[name] = entry;
    funScopes_.back().push_back(name);
}

IRVarEntry* IRGenerator::lookupVar(const std::string& name) {
    auto it = varEnv_.find(name);
    return (it != varEnv_.end()) ? &it->second : nullptr;
}

IRFunEntry* IRGenerator::lookupFun(const std::string& name) {
    auto it = funEnv_.find(name);
    return (it != funEnv_.end()) ? &it->second : nullptr;
}

//==============================================================================
// Helpers
//==============================================================================

ExpPtr IRGenerator::staticLinkChain(translate::LevelPtr fromLevel, translate::LevelPtr toLevel) {
    // Start with the frame pointer
    ExpPtr fp = TEMP(currentLevel_->frame()->framePointer());

    // Follow static links up to the target level
    auto level = fromLevel;
    while (level != toLevel && level->parent()) {
        // Get static link (first formal parameter)
        auto formals = level->frame()->formals();
        if (!formals.empty()) {
            fp = accessToExp(formals[0].get(), fp, wordSize());
        }
        level = level->parent();
    }

    return fp;
}

ExpPtr IRGenerator::accessVar(const IRVarEntry& var) {
    ExpPtr fp = staticLinkChain(currentLevel_, var.level);
    return accessToExp(var.access.get(), fp, wordSize());
}

Label IRGenerator::stringLiteral(const std::string& s) {
    Label label = tempFactory_.newLabel();
    addFragment(std::make_shared<StringFragment>(label, s));
    return label;
}

void IRGenerator::addFragment(FragmentPtr frag) { fragments_.push_back(std::move(frag)); }

StmPtr IRGenerator::procEntryExit(StmPtr body) {
    // For now, just return the body
    // Later this will add prologue/epilogue
    return body;
}

void IRGenerator::initBuiltins() {
    // Built-in functions are at the outermost level with external labels
    auto outerLevel = currentLevel_;

    // print(s: string)
    addFun("print", IRFunEntry(outerLevel, tempFactory_.namedLabel("print")));
    // printi(i: int)
    addFun("printi", IRFunEntry(outerLevel, tempFactory_.namedLabel("printi")));
    // flush()
    addFun("flush", IRFunEntry(outerLevel, tempFactory_.namedLabel("flush")));
    // getchar(): string
    addFun("getchar", IRFunEntry(outerLevel, tempFactory_.namedLabel("getchar")));
    // ord(s: string): int
    addFun("ord", IRFunEntry(outerLevel, tempFactory_.namedLabel("ord")));
    // chr(i: int): string
    addFun("chr", IRFunEntry(outerLevel, tempFactory_.namedLabel("chr")));
    // size(s: string): int
    addFun("size", IRFunEntry(outerLevel, tempFactory_.namedLabel("size")));
    // substring(s: string, first: int, n: int): string
    addFun("substring", IRFunEntry(outerLevel, tempFactory_.namedLabel("substring")));
    // concat(s1: string, s2: string): string
    addFun("concat", IRFunEntry(outerLevel, tempFactory_.namedLabel("concat")));
    // not(i: int): int
    addFun("not", IRFunEntry(outerLevel, tempFactory_.namedLabel("not")));
    // exit(i: int)
    addFun("exit", IRFunEntry(outerLevel, tempFactory_.namedLabel("exit")));
}

//==============================================================================
// Expression Visitors
//==============================================================================

TransExpPtr IRGenerator::visit(ast::NilExpr*) { return makeEx(CONST(0)); }

TransExpPtr IRGenerator::visit(ast::IntExpr* expr) { return makeEx(CONST(expr->value)); }

TransExpPtr IRGenerator::visit(ast::StringExpr* expr) {
    Label label = stringLiteral(expr->value);
    return makeEx(NAME(label));
}

TransExpPtr IRGenerator::visit(ast::VarExpr* expr) {
    switch (expr->var_kind) {
        case ast::VarExpr::VarKind::SIMPLE: {
            auto* entry = lookupVar(expr->name);
            if (!entry) {
                // Should have been caught by semantic analysis
                return makeEx(CONST(0));
            }
            return makeEx(accessVar(*entry));
        }
        case ast::VarExpr::VarKind::FIELD: {
            // record.field -> MEM(record_addr + field_offset)
            auto baseExp = expr->var->accept(*this);
            ExpPtr base = baseExp->unEx(tempFactory_);

            // Field offset would come from type info (simplified here)
            // For now, assume field index * word_size
            int offset = 0;  // TODO: get from type info

            return makeEx(MEM(BINOP(BinOp::PLUS, base, CONST(offset))));
        }
        case ast::VarExpr::VarKind::SUBSCRIPT: {
            // array[index] -> MEM(array_addr + index * word_size)
            auto baseExp = expr->var->accept(*this);
            auto indexExp = expr->index->accept(*this);

            ExpPtr base = baseExp->unEx(tempFactory_);
            ExpPtr index = indexExp->unEx(tempFactory_);

            return makeEx(
                MEM(BINOP(BinOp::PLUS, base, BINOP(BinOp::MUL, index, CONST(wordSize())))));
        }
    }

    return makeEx(CONST(0));
}

TransExpPtr IRGenerator::visit(ast::CallExpr* expr) {
    auto* funEntry = lookupFun(expr->func);
    if (!funEntry) {
        return makeEx(CONST(0));
    }

    ExpList args;

    // Add static link if calling a nested function
    if (funEntry->level && funEntry->level->parent()) {
        // Pass static link to the parent of the called function
        args.push_back(staticLinkChain(currentLevel_, funEntry->level->parent()));
    }

    // Translate arguments
    for (auto& arg : expr->args) {
        auto argExp = arg->accept(*this);
        args.push_back(argExp->unEx(tempFactory_));
    }

    return makeEx(CALL(NAME(funEntry->label), std::move(args)));
}

TransExpPtr IRGenerator::visit(ast::OpExpr* expr) {
    auto leftExp = expr->left->accept(*this);
    auto rightExp = expr->right->accept(*this);

    ExpPtr left = leftExp->unEx(tempFactory_);
    ExpPtr right = rightExp->unEx(tempFactory_);

    switch (expr->oper) {
        case ast::OpExpr::Op::PLUS:
            return makeEx(BINOP(BinOp::PLUS, left, right));
        case ast::OpExpr::Op::MINUS:
            return makeEx(BINOP(BinOp::MINUS, left, right));
        case ast::OpExpr::Op::TIMES:
            return makeEx(BINOP(BinOp::MUL, left, right));
        case ast::OpExpr::Op::DIVIDE:
            return makeEx(BINOP(BinOp::DIV, left, right));

        case ast::OpExpr::Op::EQ:
            return makeCx(
                [left, right](Label t, Label f) { return CJUMP(RelOp::EQ, left, right, t, f); });
        case ast::OpExpr::Op::NEQ:
            return makeCx(
                [left, right](Label t, Label f) { return CJUMP(RelOp::NE, left, right, t, f); });
        case ast::OpExpr::Op::LT:
            return makeCx(
                [left, right](Label t, Label f) { return CJUMP(RelOp::LT, left, right, t, f); });
        case ast::OpExpr::Op::LE:
            return makeCx(
                [left, right](Label t, Label f) { return CJUMP(RelOp::LE, left, right, t, f); });
        case ast::OpExpr::Op::GT:
            return makeCx(
                [left, right](Label t, Label f) { return CJUMP(RelOp::GT, left, right, t, f); });
        case ast::OpExpr::Op::GE:
            return makeCx(
                [left, right](Label t, Label f) { return CJUMP(RelOp::GE, left, right, t, f); });

        case ast::OpExpr::Op::AND: {
            // a & b => if a then b else 0
            Label z = tempFactory_.newLabel();
            return makeCx([this, leftExp, rightExp, z](Label t, Label f) mutable {
                return SEQ({leftExp->unCx(z, f, tempFactory_), LABEL(z),
                            rightExp->unCx(t, f, tempFactory_)});
            });
        }
        case ast::OpExpr::Op::OR: {
            // a | b => if a then 1 else b
            Label z = tempFactory_.newLabel();
            return makeCx([this, leftExp, rightExp, z](Label t, Label f) mutable {
                return SEQ({leftExp->unCx(t, z, tempFactory_), LABEL(z),
                            rightExp->unCx(t, f, tempFactory_)});
            });
        }
    }

    return makeEx(CONST(0));
}

TransExpPtr IRGenerator::visit(ast::RecordExpr* expr) {
    int numFields = static_cast<int>(expr->fields.size());

    // Allocate record: call runtime function
    Temp r = tempFactory_.newTemp();

    StmPtr initStm = MOVE(TEMP(r), CALL(NAME(tempFactory_.namedLabel("allocRecord")),
                                        {CONST(numFields * wordSize())}));

    // Initialize fields
    int offset = 0;
    for (auto& field : expr->fields) {
        auto valExp = field.second->accept(*this);
        ExpPtr val = valExp->unEx(tempFactory_);

        initStm = SEQ(initStm, MOVE(MEM(BINOP(BinOp::PLUS, TEMP(r), CONST(offset))), val));

        offset += wordSize();
    }

    return makeEx(ESEQ(initStm, TEMP(r)));
}

TransExpPtr IRGenerator::visit(ast::ArrayExpr* expr) {
    auto sizeExp = expr->size->accept(*this);
    auto initExp = expr->init->accept(*this);

    ExpPtr size = sizeExp->unEx(tempFactory_);
    ExpPtr init = initExp->unEx(tempFactory_);

    // Call runtime function to allocate and initialize array
    return makeEx(CALL(NAME(tempFactory_.namedLabel("initArray")), {size, init}));
}

TransExpPtr IRGenerator::visit(ast::AssignExpr* expr) {
    // First, get the value expression
    auto valExp = expr->expr->accept(*this);
    ExpPtr val = valExp->unEx(tempFactory_);

    // Get the lvalue (destination)
    // The var is also a VarExpr which we need to translate
    auto varExpr = dynamic_cast<ast::VarExpr*>(expr->var.get());
    if (!varExpr) {
        return makeNx(EXP(CONST(0)));
    }

    ExpPtr dst;
    switch (varExpr->var_kind) {
        case ast::VarExpr::VarKind::SIMPLE: {
            auto* entry = lookupVar(varExpr->name);
            if (entry) {
                dst = accessVar(*entry);
            } else {
                dst = CONST(0);
            }
            break;
        }
        case ast::VarExpr::VarKind::FIELD: {
            auto baseExp = varExpr->var->accept(*this);
            ExpPtr base = baseExp->unEx(tempFactory_);
            int offset = 0;  // TODO: get from type info
            dst = MEM(BINOP(BinOp::PLUS, base, CONST(offset)));
            break;
        }
        case ast::VarExpr::VarKind::SUBSCRIPT: {
            auto baseExp = varExpr->var->accept(*this);
            auto indexExp = varExpr->index->accept(*this);
            ExpPtr base = baseExp->unEx(tempFactory_);
            ExpPtr index = indexExp->unEx(tempFactory_);
            dst = MEM(BINOP(BinOp::PLUS, base, BINOP(BinOp::MUL, index, CONST(wordSize()))));
            break;
        }
    }

    return makeNx(MOVE(dst, val));
}

TransExpPtr IRGenerator::visit(ast::IfExpr* expr) {
    auto testExp = expr->test->accept(*this);
    auto thenExp = expr->then_clause->accept(*this);

    Label t = tempFactory_.newLabel();
    Label f = tempFactory_.newLabel();
    Label join = tempFactory_.newLabel();

    if (expr->else_clause) {
        // if-then-else with value
        auto elseExp = expr->else_clause->accept(*this);

        Temp r = tempFactory_.newTemp();

        return makeEx(
            ESEQ(SEQ({testExp->unCx(t, f, tempFactory_), LABEL(t),
                      MOVE(TEMP(r), thenExp->unEx(tempFactory_)), JUMP(join), LABEL(f),
                      MOVE(TEMP(r), elseExp->unEx(tempFactory_)), JUMP(join), LABEL(join)}),
                 TEMP(r)));
    } else {
        // if-then (no value)
        return makeNx(SEQ(
            {testExp->unCx(t, f, tempFactory_), LABEL(t), thenExp->unNx(tempFactory_), LABEL(f)}));
    }
}

TransExpPtr IRGenerator::visit(ast::WhileExpr* expr) {
    Label test = tempFactory_.newLabel();
    Label body = tempFactory_.newLabel();
    Label done = tempFactory_.newLabel();

    breakLabels_.push_back(done);

    auto testExp = expr->test->accept(*this);
    auto bodyExp = expr->body->accept(*this);

    breakLabels_.pop_back();

    return makeNx(SEQ({LABEL(test), testExp->unCx(body, done, tempFactory_), LABEL(body),
                       bodyExp->unNx(tempFactory_), JUMP(test), LABEL(done)}));
}

TransExpPtr IRGenerator::visit(ast::ForExpr* expr) {
    Label body = tempFactory_.newLabel();
    Label incr = tempFactory_.newLabel();
    Label done = tempFactory_.newLabel();

    // Allocate loop variable
    auto access = currentLevel_->frame()->allocLocal(expr->escape);
    IRVarEntry varEntry(currentLevel_, access);

    beginScope();
    addVar(expr->var, varEntry);

    auto loExp = expr->lo->accept(*this);
    auto hiExp = expr->hi->accept(*this);

    ExpPtr varAddr = accessVar(varEntry);
    Temp limit = tempFactory_.newTemp();

    breakLabels_.push_back(done);
    auto bodyExp = expr->body->accept(*this);
    breakLabels_.pop_back();

    endScope();

    return makeNx(
        SEQ({// Initialize loop variable and limit
             MOVE(varAddr, loExp->unEx(tempFactory_)), MOVE(TEMP(limit), hiExp->unEx(tempFactory_)),
             // Check initial condition
             CJUMP(RelOp::LE, varAddr, TEMP(limit), body, done), LABEL(body),
             bodyExp->unNx(tempFactory_),
             // Check if we're at the limit before incrementing
             CJUMP(RelOp::LT, varAddr, TEMP(limit), incr, done), LABEL(incr),
             MOVE(varAddr, BINOP(BinOp::PLUS, varAddr, CONST(1))), JUMP(body), LABEL(done)}));
}

TransExpPtr IRGenerator::visit(ast::BreakExpr*) {
    if (breakLabels_.empty()) {
        return makeNx(nullptr);
    }
    return makeNx(JUMP(breakLabels_.back()));
}

TransExpPtr IRGenerator::visit(ast::LetExpr* expr) {
    beginScope();

    StmPtr declStm = nullptr;
    for (auto& decl : expr->decls) {
        auto declExp = decl->accept(*this);
        if (declExp) {
            declStm = SEQ(declStm, declExp->unNx(tempFactory_));
        }
    }

    // Body is a list of expressions, we need to evaluate all and return last
    StmPtr bodyStm = nullptr;
    TransExpPtr lastExp = makeEx(CONST(0));

    for (size_t i = 0; i < expr->body.size(); ++i) {
        auto e = expr->body[i]->accept(*this);
        if (i < expr->body.size() - 1) {
            bodyStm = SEQ(bodyStm, e->unNx(tempFactory_));
        } else {
            lastExp = e;
        }
    }

    endScope();

    StmPtr allStm = SEQ(declStm, bodyStm);
    if (allStm) {
        return makeEx(ESEQ(allStm, lastExp->unEx(tempFactory_)));
    }
    return lastExp;
}

TransExpPtr IRGenerator::visit(ast::SeqExpr* expr) {
    if (expr->exprs.empty()) {
        return makeNx(nullptr);
    }

    StmPtr stm = nullptr;

    for (size_t i = 0; i < expr->exprs.size() - 1; ++i) {
        auto e = expr->exprs[i]->accept(*this);
        stm = SEQ(stm, e->unNx(tempFactory_));
    }

    // Last expression is the value
    auto lastExp = expr->exprs.back()->accept(*this);

    if (stm) {
        return makeEx(ESEQ(stm, lastExp->unEx(tempFactory_)));
    }
    return lastExp;
}

//==============================================================================
// Declaration Visitors
//==============================================================================

TransExpPtr IRGenerator::visit(ast::TypeDecl*) {
    // Type declarations don't generate IR
    return nullptr;
}

TransExpPtr IRGenerator::visit(ast::VarDecl* decl) {
    // Allocate variable
    auto access = currentLevel_->frame()->allocLocal(decl->escape);
    addVar(decl->name, IRVarEntry(currentLevel_, access));

    // Initialize variable
    auto initExp = decl->init->accept(*this);
    ExpPtr varAddr =
        accessToExp(access.get(), TEMP(currentLevel_->frame()->framePointer()), wordSize());

    return makeNx(MOVE(varAddr, initExp->unEx(tempFactory_)));
}

TransExpPtr IRGenerator::visit(ast::FunctionDecl* decl) {
    // Single function declaration
    Label funcLabel = tempFactory_.namedLabel(decl->name);

    // Collect escape info for parameters
    std::vector<bool> formals;
    formals.push_back(true);  // Static link always escapes
    for (auto& param : decl->params) {
        formals.push_back(param->escape);
    }

    // Create new level for function
    auto funcLevel = translate::Level::newLevel(currentLevel_, funcLabel, formals, *frameFactory_);

    addFun(decl->name, IRFunEntry(funcLevel, funcLabel));

    // Translate function body
    auto savedLevel = currentLevel_;
    currentLevel_ = funcLevel;

    beginScope();

    // Add parameters to scope
    auto frameFormals = currentLevel_->frame()->formals();
    for (size_t i = 0; i < decl->params.size(); ++i) {
        // Skip static link (index 0)
        addVar(decl->params[i]->name, IRVarEntry(currentLevel_, frameFormals[i + 1]));
    }

    // Translate body
    auto bodyExp = decl->body->accept(*this);

    // Move result to return register
    StmPtr bodyStm;
    if (decl->result_type.empty()) {
        // Procedure (no return value)
        bodyStm = bodyExp->unNx(tempFactory_);
    } else {
        // Function (has return value)
        bodyStm = MOVE(TEMP(currentLevel_->frame()->returnValue()), bodyExp->unEx(tempFactory_));
    }

    endScope();

    // Add fragment
    addFragment(std::make_shared<ProcFragment>(procEntryExit(bodyStm), currentLevel_->frame()));

    currentLevel_ = savedLevel;

    return nullptr;
}

}  // namespace ir
}  // namespace tiger
