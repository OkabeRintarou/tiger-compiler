#include "semantic/semantic_analyzer.hpp"

#include <cassert>
#include <unordered_set>

#include "ast/ast.hpp"

namespace tiger {
namespace semantic {

SemanticAnalyzer::SemanticAnalyzer(TypeContext& typeCtx)
    : env_(typeCtx), currentReturnType_(nullptr) {}

TypePtr SemanticAnalyzer::analyze(ast::ExprPtr expr) { return expr->accept(*this); }

void SemanticAnalyzer::error(const std::string& msg, int line, int column) {
    throw SemanticError(msg, line, column);
}

TypePtr SemanticAnalyzer::checkTypeEquals(TypePtr expected, TypePtr actual,
                                          const std::string& errorMsg, int line, int column) {
    if (!expected->equals(actual.get())) {
        error(errorMsg + " (expected " + expected->toString() + ", got " + actual->toString() + ")",
              line, column);
    }
    return expected;
}

TypePtr SemanticAnalyzer::checkAssignable(TypePtr varType, TypePtr exprType,
                                          const std::string& varName, int line, int column) {
    // Check if expression type can be assigned to variable type
    if (!varType->equals(exprType.get())) {
        // Special case: nil can be assigned to any record type
        if (exprType->isNil() && varType->isRecord()) {
            return varType;
        }
        error("Type mismatch in assignment to '" + varName + "' (expected " + varType->toString() +
                  ", got " + exprType->toString() + ")",
              line, column);
    }
    return varType;
}

TypePtr SemanticAnalyzer::translateType(ast::Type* astType) {
    if (!astType) {
        return nullptr;
    }

    // Handle NameType - look up in type environment
    if (auto nameType = dynamic_cast<ast::NameType*>(astType)) {
        TypePtr type = env_.lookupType(nameType->name);
        if (!type) {
            error("Undefined type: " + nameType->name, 0, 0);
        }
        return type;
    }

    // Handle RecordType - create new record type
    if (auto recordType = dynamic_cast<ast::RecordType*>(astType)) {
        auto newRecord = env_.getTypeContext().createRecordType();

        for (const auto& field : recordType->fields) {
            // Translate field type
            TypePtr fieldType = env_.lookupType(field->type_id);
            if (!fieldType) {
                error("Unknown field type in record: " + field->type_id, 0, 0);
            }
            newRecord->addField(field->name, fieldType);
        }

        return newRecord;
    }

    // Handle ArrayType - create new array type
    if (auto arrayType = dynamic_cast<ast::ArrayType*>(astType)) {
        TypePtr elementType = env_.lookupType(arrayType->element_type);
        if (!elementType) {
            error("Undefined array element type: " + arrayType->element_type, 0, 0);
        }
        return env_.getTypeContext().createArrayType(elementType);
    }

    return nullptr;
}

// ========== Expression Visitors ==========

TypePtr SemanticAnalyzer::visit(ast::VarExpr* expr) {
    // Handle field access (e.g., record.field)
    if (expr->var_kind == ast::VarExpr::VarKind::FIELD) {
        // First, get the type of the left side (the record variable)
        TypePtr varType = expr->var->accept(*this);

        if (!varType) {
            error("Field access on null type", 0, 0);
        }

        const RecordType* recordType = varType->asRecord();
        if (!recordType) {
            error("Field access on non-record type: " + varType->toString(), 0, 0);
        }

        // In FIELD mode, expr->name contains the field name
        TypePtr fieldType = recordType->getFieldType(expr->name);
        if (!fieldType) {
            error("Record has no field named '" + expr->name + "'", 0, 0);
        }

        return fieldType;
    }

    // Handle array subscript (e.g., array[index])
    if (expr->var_kind == ast::VarExpr::VarKind::SUBSCRIPT) {
        // First, get the type of the left side (the array variable)
        TypePtr varType = expr->var->accept(*this);

        if (!varType || !varType->isArray()) {
            error("Array subscript on non-array type", 0, 0);
        }

        // Index expression must be int
        TypePtr indexType = expr->index->accept(*this);
        checkTypeEquals(env_.getTypeContext().getIntType(), indexType,
                        "Array index must be integer", 0, 0);

        // Return element type
        const ArrayType* arrayType = varType->asArray();
        if (!arrayType) {
            error("Array subscript on non-array type", 0, 0);
        }

        return arrayType->getElementType();
    }

    // Simple variable - look up in value environment
    VarEntry* varEntry = env_.lookupVar(expr->name);
    if (!varEntry) {
        // Check if it's a function instead (better error message)
        FuncEntry* funcEntry = env_.lookupFunc(expr->name);
        if (funcEntry) {
            error("'" + expr->name + "' is a function, not a variable", 0, 0);
        }
        error("Undefined variable: " + expr->name, 0, 0);
    }

    return varEntry->getType();
}

TypePtr SemanticAnalyzer::visit(ast::NilExpr*) { return env_.getTypeContext().getNilType(); }

TypePtr SemanticAnalyzer::visit(ast::IntExpr*) { return env_.getTypeContext().getIntType(); }

TypePtr SemanticAnalyzer::visit(ast::StringExpr*) { return env_.getTypeContext().getStringType(); }

TypePtr SemanticAnalyzer::visit(ast::CallExpr* expr) {
    // Look up function
    FuncEntry* funcEntry = env_.lookupFunc(expr->func);
    if (!funcEntry) {
        // Check if it's a variable instead (better error message)
        VarEntry* varEntry = env_.lookupVar(expr->func);
        if (varEntry) {
            error("'" + expr->func + "' is a variable, not a function", 0, 0);
        }
        error("Undefined function: " + expr->func, 0, 0);
    }

    const auto& paramTypes = funcEntry->getParamTypes();

    // Check argument count
    if (expr->args.size() != paramTypes.size()) {
        error("Function '" + expr->func + "' expects " + std::to_string(paramTypes.size()) +
                  " arguments, got " + std::to_string(expr->args.size()),
              0, 0);
    }

    // Check argument types
    for (size_t i = 0; i < expr->args.size(); ++i) {
        TypePtr argType = expr->args[i]->accept(*this);
        checkTypeEquals(paramTypes[i], argType,
                        "Argument type mismatch in call to '" + expr->func + "'", 0, 0);
    }

    return funcEntry->getReturnType();
}

TypePtr SemanticAnalyzer::visit(ast::OpExpr* expr) {
    TypePtr leftType = expr->left->accept(*this);
    TypePtr rightType = expr->right->accept(*this);
    auto& typeCtx = env_.getTypeContext();

    // Arithmetic operators: +, -, *, /
    if (expr->oper == ast::OpExpr::Op::PLUS || expr->oper == ast::OpExpr::Op::MINUS ||
        expr->oper == ast::OpExpr::Op::TIMES || expr->oper == ast::OpExpr::Op::DIVIDE) {
        checkTypeEquals(typeCtx.getIntType(), leftType,
                        "Left operand of arithmetic operator must be int", 0, 0);
        checkTypeEquals(typeCtx.getIntType(), rightType,
                        "Right operand of arithmetic operator must be int", 0, 0);
        return typeCtx.getIntType();
    }

    // Comparison operators: =, <>, <, >, <=, >=
    if (expr->oper == ast::OpExpr::Op::EQ || expr->oper == ast::OpExpr::Op::NEQ ||
        expr->oper == ast::OpExpr::Op::LT || expr->oper == ast::OpExpr::Op::GT ||
        expr->oper == ast::OpExpr::Op::LE || expr->oper == ast::OpExpr::Op::GE) {
        // Both operands must have the same type
        checkTypeEquals(leftType, rightType, "Comparison operands must have the same type", 0, 0);
        return typeCtx.getIntType();
    }

    // Logical operators: &, |
    if (expr->oper == ast::OpExpr::Op::AND || expr->oper == ast::OpExpr::Op::OR) {
        checkTypeEquals(typeCtx.getIntType(), leftType,
                        "Left operand of logical operator must be int", 0, 0);
        checkTypeEquals(typeCtx.getIntType(), rightType,
                        "Right operand of logical operator must be int", 0, 0);
        return typeCtx.getIntType();
    }

    return typeCtx.getIntType();
}

TypePtr SemanticAnalyzer::visit(ast::RecordExpr* expr) {
    // Look up record type
    TypePtr type = env_.lookupType(expr->type_id);
    if (!type) {
        error("Undefined type: " + expr->type_id, 0, 0);
    }

    RecordType* recordType = type->asRecord();
    if (recordType == nullptr) {
        error("Type '" + expr->type_id + "' is not a record type", 0, 0);
    }

    const auto& recordFields = recordType->getFields();

    // Check field count
    if (expr->fields.size() != recordFields.size()) {
        error("Record creation expects " + std::to_string(recordFields.size()) + " fields, got " +
                  std::to_string(expr->fields.size()),
              0, 0);
    }

    // Check each field
    for (size_t i = 0; i < expr->fields.size(); ++i) {
        const auto& field = expr->fields[i];

        // Check field name and order
        if (field.first != recordFields[i].name) {
            error("Field '" + field.first + "' not found or wrong order in record type", 0, 0);
        }

        // Check field type
        TypePtr fieldType = field.second->accept(*this);
        checkTypeEquals(recordFields[i].type, fieldType,
                        "Type mismatch for field '" + field.first + "' in record creation", 0, 0);
    }

    return type;
}

TypePtr SemanticAnalyzer::visit(ast::ArrayExpr* expr) {
    // Look up array type
    TypePtr type = env_.lookupType(expr->type_id);
    if (!type) {
        error("Undefined type: " + expr->type_id, 0, 0);
    }

    ArrayType* arrayType = type->asArray();
    if (arrayType == nullptr) {
        error("Type '" + expr->type_id + "' is not an array type", 0, 0);
    }

    TypePtr elemType = arrayType->getElementType();

    // Size must be integer
    TypePtr sizeType = expr->size->accept(*this);
    checkTypeEquals(env_.getTypeContext().getIntType(), sizeType, "Array size must be integer", 0,
                    0);

    // Initializer must match element type
    TypePtr initType = expr->init->accept(*this);
    checkAssignable(elemType, initType, "array initialization", 0, 0);

    return type;
}

TypePtr SemanticAnalyzer::visit(ast::AssignExpr* expr) {
    // Get variable type (also validates lvalue)
    TypePtr varType = expr->var->accept(*this);

    // Get expression type
    TypePtr exprType = expr->expr->accept(*this);

    // For field access, varType is the field type
    // For array subscript, varType is the element type
    // For simple variable, varType is the variable type
    if (auto varExpr = dynamic_cast<ast::VarExpr*>(expr->var.get())) {
        if (varExpr->var_kind == ast::VarExpr::VarKind::SIMPLE) {
            VarEntry* varEntry = env_.lookupVar(varExpr->name);
            if (varEntry && varEntry->isReadOnly()) {
                error("Cannot assign to loop variable '" + varExpr->name + "'", 0, 0);
            }
        }
    }

    // Check type compatibility
    checkAssignable(varType, exprType, "assignment", 0, 0);

    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(ast::IfExpr* expr) {
    // Test condition must be int
    TypePtr testType = expr->test->accept(*this);
    checkTypeEquals(env_.getTypeContext().getIntType(), testType, "If condition must be integer", 0,
                    0);

    // Check then clause
    TypePtr thenType = expr->then_clause->accept(*this);

    // If else clause exists, check it
    if (expr->else_clause) {
        TypePtr elseType = expr->else_clause->accept(*this);
        // then and else must have the same type
        checkTypeEquals(thenType, elseType, "If-then-else branches must have the same type", 0, 0);
        return thenType;
    }

    // No else clause - then clause must produce no value (void)
    checkTypeEquals(env_.getTypeContext().getVoidType(), thenType,
                    "If-then without else must produce no value", 0, 0);
    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(ast::WhileExpr* expr) {
    // Test condition must be int
    TypePtr testType = expr->test->accept(*this);
    checkTypeEquals(env_.getTypeContext().getIntType(), testType, "While condition must be integer",
                    0, 0);

    // Enter loop context (for break statement)
    env_.enterLoop();

    // Check body
    TypePtr bodyType = expr->body->accept(*this);

    // Body must produce no value (void)
    checkTypeEquals(env_.getTypeContext().getVoidType(), bodyType,
                    "While loop body must produce no value", 0, 0);

    // Exit loop context
    env_.exitLoop();

    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(ast::ForExpr* expr) {
    // Check low and high bounds (must be int)
    TypePtr loType = expr->lo->accept(*this);
    TypePtr hiType = expr->hi->accept(*this);
    checkTypeEquals(env_.getTypeContext().getIntType(), loType, "For loop lower bound must be int",
                    0, 0);
    checkTypeEquals(env_.getTypeContext().getIntType(), hiType, "For loop upper bound must be int",
                    0, 0);

    // Enter new scope for loop variable
    env_.beginScope();

    // Enter loop variable (read-only)
    env_.enterVar(expr->var, env_.getTypeContext().getIntType(), true);

    // Enter loop context
    env_.enterLoop();

    // Check body
    TypePtr bodyType = expr->body->accept(*this);

    // Exit loop context
    env_.exitLoop();

    // Exit loop variable scope
    env_.endScope();

    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(ast::BreakExpr*) {
    // Break must be inside a loop
    if (!env_.inLoop()) {
        error("break statement must be inside a loop", 0, 0);
    }

    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(ast::LetExpr* expr) {
    // Enter new scope for declarations
    env_.beginScope();

    // Process declarations in groups
    // Type declarations can only be mutually recursive if they are consecutive
    size_t i = 0;
    while (i < expr->decls.size()) {
        // Find a consecutive group of type declarations
        std::vector<ast::TypeDeclPtr> typeGroup;
        std::vector<ast::FunctionDeclPtr> functionGroup;
        while (i < expr->decls.size()) {
            if (expr->decls[i]->isTypeDecl()) {
                auto typeDecl = std::dynamic_pointer_cast<ast::TypeDecl>(expr->decls[i]);
                typeGroup.emplace_back(std::move(typeDecl));
                i++;
            } else if (expr->decls[i]->isFunctionDecl()) {
                auto funcDecl = std::dynamic_pointer_cast<ast::FunctionDecl>(expr->decls[i]);
                functionGroup.emplace_back(std::move(funcDecl));
                i++;
            } else {
                break;
            }
        }

        // Process the type declaration group
        if (!typeGroup.empty()) {
            processTypeDeclarations(typeGroup);
        }
        if (!functionGroup.empty()) {
            processFunctionDeclarations(functionGroup);
        }

        // Process non-type/non-function declarations
        while (i < expr->decls.size()) {
            if (expr->decls[i]->isTypeDecl() || expr->decls[i]->isFunctionDecl()) {
                break;  // Type declaration, start a new group
            }
            expr->decls[i]->accept(*this);
            i++;
        }
    }
    // Process body expressions
    TypePtr lastType = env_.getTypeContext().getVoidType();
    for (const auto& e : expr->body) {
        lastType = e->accept(*this);
    }

    // Exit scope
    env_.endScope();

    return lastType;
}

void SemanticAnalyzer::processTypeDeclarations(const std::vector<ast::TypeDeclPtr>& typeDecls) {
    auto& ctx = env_.getTypeContext();

    // Phase 1: Create NameType for all type declarations and register them
    for (const auto& typeDecl : typeDecls) {
        TypePtr nameType = ctx.createNameType(typeDecl->name);
        env_.enterType(typeDecl->name, nameType);
    }

    // Phase 2: Translate type definitions (now all types in the group are registered)
    for (const auto& typeDecl : typeDecls) {
        TypePtr nameType = env_.lookupType(typeDecl->name);
        if (nameType && nameType->isName()) {
            TypePtr actualType = translateType(typeDecl->type.get());
            static_cast<NameType*>(nameType.get())->bind(actualType);
        }
    }

    // Phase 3: Check cycle of mutually recursive types
    std::unordered_set<std::string> checkedNames;
    std::unordered_set<std::string> deps;
    std::vector<std::string> cycle;
    for (const auto& typeDecl : typeDecls) {
        if (checkedNames.find(typeDecl->name) != checkedNames.end()) continue;

        cycle.clear();
        deps = {typeDecl->name};
        checkedNames.insert(typeDecl->name);
        TypePtr type = env_.lookupType(typeDecl->name);
        NameType* nameType = type->asName();

        while (nameType != nullptr) {
            auto actualType = nameType->getBinding();
            if (auto actualNameType = actualType->asName(); actualNameType != nullptr) {
                const auto& depName = actualNameType->getName();
                cycle.emplace_back(depName);

                if (deps.find(depName) == deps.end()) {
                    deps.insert(depName);
                    nameType = actualNameType;
                } else {
                    auto errorMsg =
                        std::string("Find a cycle of type declaration '" + typeDecl->name + "': ");
                    for (const auto& dep : cycle) {
                        errorMsg += (" -> '" + dep + "'");
                    }
                    error(errorMsg, 0, 0);
                }
            } else {
                // dependency is not a name type, break the cycle detection
                break;
            }
        }
    }
}

void SemanticAnalyzer::processFunctionDeclarations(
    const std::vector<ast::FunctionDeclPtr>& funcDecls) {
    // Phase 1: Enter all function headers into the environment
    // This allows mutual recursion between functions in the same group
    for (const auto& funcDecl : funcDecls) {
        // Translate parameter types
        std::vector<TypePtr> paramTypes;
        for (const auto& param : funcDecl->params) {
            TypePtr paramType = env_.lookupType(param->type_id);
            if (!paramType) {
                error("Undefined parameter type: " + param->type_id, 0, 0);
            }
            paramTypes.push_back(paramType);
        }

        // Translate return type (if specified)
        TypePtr returnType = env_.getTypeContext().getVoidType();
        if (!funcDecl->result_type.empty()) {
            returnType = env_.lookupType(funcDecl->result_type);
            if (!returnType) {
                error("Undefined return type: " + funcDecl->result_type, 0, 0);
            }
        }

        // Enter function into environment
        env_.enterFunc(funcDecl->name, paramTypes, returnType);
    }

    // Phase 2: Process function bodies
    for (const auto& funcDecl : funcDecls) {
        FuncEntry* entry = env_.lookupFunc(funcDecl->name);

        // Enter new scope for function body
        env_.beginScope();

        // Save and set current return type
        TypePtr savedReturnType = currentReturnType_;
        currentReturnType_ = entry->getReturnType();

        // Enter parameters into environment
        const auto& paramTypes = entry->getParamTypes();
        for (size_t i = 0; i < funcDecl->params.size(); ++i) {
            env_.enterVar(funcDecl->params[i]->name, paramTypes[i], false);
        }

        // Analyze function body
        TypePtr bodyType = funcDecl->body->accept(*this);

        // Check return type if function has non-void return type
        if (!currentReturnType_->isVoid()) {
            checkTypeEquals(currentReturnType_, bodyType, "Function body return type mismatch", 0,
                            0);
        }

        // Restore return type
        currentReturnType_ = savedReturnType;

        // Exit function scope
        env_.endScope();
    }
}

TypePtr SemanticAnalyzer::visit(ast::SeqExpr* expr) {
    TypePtr lastType = env_.getTypeContext().getVoidType();
    for (const auto& e : expr->exprs) {
        lastType = e->accept(*this);
    }
    return lastType;
}

// ========== Declaration Visitors ==========

TypePtr SemanticAnalyzer::visit(ast::TypeDecl*) { return nullptr; }

TypePtr SemanticAnalyzer::visit(ast::VarDecl* decl) {
    // Check initializer expression
    TypePtr initType = decl->init->accept(*this);

    // If type annotation provided, check it matches
    TypePtr varType = initType;
    if (!decl->type_id.empty()) {
        varType = env_.lookupType(decl->type_id);
        if (!varType) {
            error("Undefined type in variable declaration: " + decl->type_id, 0, 0);
        }
        checkAssignable(varType, initType, decl->name, 0, 0);
    }

    // Enter variable into environment
    env_.enterVar(decl->name, varType, false);

    return nullptr;
}

TypePtr SemanticAnalyzer::visit(ast::FunctionDecl*) { return nullptr; }

}  // namespace semantic
}  // namespace tiger
