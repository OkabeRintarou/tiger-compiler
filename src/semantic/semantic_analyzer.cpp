#include "semantic/semantic_analyzer.hpp"

#include <iostream>

#include "ast/ast.hpp"

namespace tiger {
namespace semantic {

SemanticAnalyzer::SemanticAnalyzer(TypeContext& typeCtx)
    : env_(typeCtx), currentReturnType_(nullptr) {}

TypePtr SemanticAnalyzer::analyze(ExprPtr expr) { return expr->accept(*this); }

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

TypePtr SemanticAnalyzer::translateType(tiger::Type* astType) {
    if (!astType) {
        return nullptr;
    }

    // Handle NameType - look up in type environment
    if (auto nameType = dynamic_cast<tiger::NameType*>(astType)) {
        TypePtr type = env_.lookupType(nameType->name);
        if (!type) {
            error("Undefined type: " + nameType->name, 0, 0);
        }
        return type;
    }

    // Handle RecordType - create new record type
    if (auto recordType = dynamic_cast<tiger::RecordType*>(astType)) {
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
    if (auto arrayType = dynamic_cast<tiger::ArrayType*>(astType)) {
        TypePtr elementType = env_.lookupType(arrayType->element_type);
        if (!elementType) {
            error("Undefined array element type: " + arrayType->element_type, 0, 0);
        }
        return env_.getTypeContext().createArrayType(elementType);
    }

    return nullptr;
}

// ========== Expression Visitors ==========

TypePtr SemanticAnalyzer::visit(VarExpr* expr) {
    // Look up variable in value environment
    VarEntry* varEntry = env_.lookupVar(expr->name);
    if (!varEntry) {
        // Check if it's a function instead (better error message)
        FuncEntry* funcEntry = env_.lookupFunc(expr->name);
        if (funcEntry) {
            error("'" + expr->name + "' is a function, not a variable", 0, 0);
        }
        error("Undefined variable: " + expr->name, 0, 0);
    }

    // Handle field access (e.g., record.field)
    if (expr->var_kind == VarExpr::VarKind::FIELD) {
        TypePtr varType = varEntry->getType();

        if (!varType->isRecord()) {
            error("Field access on non-record type: " + varType->toString(), 0, 0);
        }

        RecordType* recordType = static_cast<RecordType*>(varType->actual());
        TypePtr fieldType = recordType->getFieldType(expr->field);
        if (!fieldType) {
            error("Record has no field named '" + expr->field + "'", 0, 0);
        }

        return fieldType;
    }

    // Handle array subscript (e.g., array[index])
    if (expr->var_kind == VarExpr::VarKind::SUBSCRIPT) {
        TypePtr varType = varEntry->getType();

        if (!varType->isArray()) {
            error("Array subscript on non-array type: " + varType->toString(), 0, 0);
        }

        // Index expression must be int
        TypePtr indexType = expr->index->accept(*this);
        checkTypeEquals(env_.getTypeContext().getIntType(), indexType,
                        "Array index must be integer", 0, 0);

        // Return element type
        ArrayType* arrayType = static_cast<ArrayType*>(varType->actual());
        return arrayType->getElementType();
    }

    // Simple variable
    return varEntry->getType();
}

TypePtr SemanticAnalyzer::visit(NilExpr* expr) { return env_.getTypeContext().getNilType(); }

TypePtr SemanticAnalyzer::visit(IntExpr* expr) { return env_.getTypeContext().getIntType(); }

TypePtr SemanticAnalyzer::visit(StringExpr* expr) { return env_.getTypeContext().getStringType(); }

TypePtr SemanticAnalyzer::visit(CallExpr* expr) {
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

TypePtr SemanticAnalyzer::visit(OpExpr* expr) {
    TypePtr leftType = expr->left->accept(*this);
    TypePtr rightType = expr->right->accept(*this);
    auto& typeCtx = env_.getTypeContext();

    // Arithmetic operators: +, -, *, /
    if (expr->oper == OpExpr::Op::PLUS || expr->oper == OpExpr::Op::MINUS ||
        expr->oper == OpExpr::Op::TIMES || expr->oper == OpExpr::Op::DIVIDE) {
        checkTypeEquals(typeCtx.getIntType(), leftType,
                        "Left operand of arithmetic operator must be int", 0, 0);
        checkTypeEquals(typeCtx.getIntType(), rightType,
                        "Right operand of arithmetic operator must be int", 0, 0);
        return typeCtx.getIntType();
    }

    // Comparison operators: =, <>, <, >, <=, >=
    if (expr->oper == OpExpr::Op::EQ || expr->oper == OpExpr::Op::NEQ ||
        expr->oper == OpExpr::Op::LT || expr->oper == OpExpr::Op::GT ||
        expr->oper == OpExpr::Op::LE || expr->oper == OpExpr::Op::GE) {
        // Both operands must have the same type
        checkTypeEquals(leftType, rightType, "Comparison operands must have the same type", 0, 0);
        return typeCtx.getIntType();
    }

    // Logical operators: &, |
    if (expr->oper == OpExpr::Op::AND || expr->oper == OpExpr::Op::OR) {
        checkTypeEquals(typeCtx.getIntType(), leftType,
                        "Left operand of logical operator must be int", 0, 0);
        checkTypeEquals(typeCtx.getIntType(), rightType,
                        "Right operand of logical operator must be int", 0, 0);
        return typeCtx.getIntType();
    }

    return typeCtx.getIntType();
}

TypePtr SemanticAnalyzer::visit(RecordExpr* expr) {
    // Look up record type
    TypePtr type = env_.lookupType(expr->type_id);
    if (!type) {
        error("Undefined type: " + expr->type_id, 0, 0);
    }

    if (!type->isRecord()) {
        error("Type '" + expr->type_id + "' is not a record type", 0, 0);
    }

    RecordType* recordType = static_cast<RecordType*>(type->actual());
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

TypePtr SemanticAnalyzer::visit(ArrayExpr* expr) {
    // Look up array type
    TypePtr type = env_.lookupType(expr->type_id);
    if (!type) {
        error("Undefined type: " + expr->type_id, 0, 0);
    }

    if (!type->isArray()) {
        error("Type '" + expr->type_id + "' is not an array type", 0, 0);
    }

    ArrayType* arrayType = static_cast<ArrayType*>(type->actual());
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

TypePtr SemanticAnalyzer::visit(AssignExpr* expr) {
    // Get variable type (also validates lvalue)
    TypePtr varType = expr->var->accept(*this);

    // Get expression type
    TypePtr exprType = expr->expr->accept(*this);

    // For field access, varType is the field type
    // For array subscript, varType is the element type
    // For simple variable, varType is the variable type
    if (auto varExpr = dynamic_cast<VarExpr*>(expr->var.get())) {
        if (varExpr->var_kind == VarExpr::VarKind::SIMPLE) {
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

TypePtr SemanticAnalyzer::visit(IfExpr* expr) {
    // Test condition must be int
    TypePtr testType = expr->test->accept(*this);
    checkTypeEquals(env_.getTypeContext().getIntType(), testType, "If condition must be integer", 0,
                    0);

    // Check then clause
    TypePtr thenType = expr->then_clause->accept(*this);

    // If else clause exists, check it
    if (expr->else_clause) {
        TypePtr elseType = expr->else_clause->accept(*this);
        // then and else don't need to match in Tiger
        return elseType;
    }

    // No else clause - returns void
    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(WhileExpr* expr) {
    // Test condition must be int
    TypePtr testType = expr->test->accept(*this);
    checkTypeEquals(env_.getTypeContext().getIntType(), testType, "While condition must be integer",
                    0, 0);

    // Enter loop context (for break statement)
    env_.enterLoop();

    // Check body
    TypePtr bodyType = expr->body->accept(*this);

    // Exit loop context
    env_.exitLoop();

    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(ForExpr* expr) {
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

TypePtr SemanticAnalyzer::visit(BreakExpr* expr) {
    // Break must be inside a loop
    if (!env_.inLoop()) {
        error("break statement must be inside a loop", 0, 0);
    }

    return env_.getTypeContext().getVoidType();
}

TypePtr SemanticAnalyzer::visit(LetExpr* expr) {
    // Enter new scope for declarations
    env_.beginScope();

    // Process declarations
    for (const auto& decl : expr->decls) {
        decl->accept(*this);
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

TypePtr SemanticAnalyzer::visit(SeqExpr* expr) {
    TypePtr lastType = env_.getTypeContext().getVoidType();
    for (const auto& e : expr->exprs) {
        lastType = e->accept(*this);
    }
    return lastType;
}

// ========== Declaration Visitors ==========

TypePtr SemanticAnalyzer::visit(TypeDecl* decl) {
    // Translate the type
    TypePtr type = translateType(decl->type.get());

    // Enter type into environment
    env_.enterType(decl->name, type);

    return nullptr;
}

TypePtr SemanticAnalyzer::visit(VarDecl* decl) {
    // Check initializer expression
    TypePtr initType = decl->init->accept(*this);

    // If type annotation provided, check it matches
    TypePtr varType = initType;
    if (!decl->type_id.empty()) {
        varType = env_.lookupType(decl->type_id);
        if (!varType) {
            error("Undefined type in variable declaration: " + decl->type_id, 0, 0);
        }
        checkAssignable(varType, initType, "variable declaration", 0, 0);
    }

    // Enter variable into environment
    env_.enterVar(decl->name, varType, false);

    return nullptr;
}

TypePtr SemanticAnalyzer::visit(FunctionDecl* decl) {
    // Translate parameter types
    std::vector<TypePtr> paramTypes;
    for (const auto& param : decl->params) {
        TypePtr paramType = env_.lookupType(param->type_id);
        if (!paramType) {
            error("Undefined parameter type: " + param->type_id, 0, 0);
        }
        paramTypes.push_back(paramType);
    }

    // Translate return type (if specified)
    TypePtr returnType = env_.getTypeContext().getVoidType();
    if (!decl->result_type.empty()) {
        returnType = env_.lookupType(decl->result_type);
        if (!returnType) {
            error("Undefined return type: " + decl->result_type, 0, 0);
        }
    }

    // Enter function into environment
    env_.enterFunc(decl->name, paramTypes, returnType);

    // Enter new scope for function body
    env_.beginScope();

    // Save and set current return type
    TypePtr savedReturnType = currentReturnType_;
    currentReturnType_ = returnType;

    // Enter parameters into environment
    for (size_t i = 0; i < decl->params.size(); ++i) {
        env_.enterVar(decl->params[i]->name, paramTypes[i], false);
    }

    // Analyze function body
    TypePtr bodyType = decl->body->accept(*this);

    // Check return type if function has non-void return type
    if (!returnType->isVoid()) {
        checkTypeEquals(returnType, bodyType, "Function body return type mismatch", 0, 0);
    }

    // Restore return type
    currentReturnType_ = savedReturnType;

    // Exit function scope
    env_.endScope();

    return nullptr;
}

}  // namespace semantic
}  // namespace tiger
