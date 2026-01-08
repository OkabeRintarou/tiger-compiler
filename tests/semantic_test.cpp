#include <gtest/gtest.h>

#include <fstream>
#include <iterator>

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "semantic/types.hpp"

namespace tiger {
namespace test {
using namespace tiger::ast;

using semantic::SemanticAnalyzer;
using semantic::SemanticError;
using semantic::TypeContext;
using semantic::TypePtr;

class SemanticTest : public ::testing::Test {
protected:
    // Helper to parse and analyze a Tiger program
    TypePtr analyze(const std::string& source) {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        Parser parser(tokens);
        ExprPtr ast = parser.parse();

        SemanticAnalyzer analyzer(typeCtx_);
        return analyzer.analyze(ast);
    }

    // Helper to analyze a Tiger program from file
    TypePtr analyzeFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        std::string source((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return analyze(source);
    }

    // Helper to check if analysis throws an error
    bool hasError(const std::string& source) {
        try {
            analyze(source);
            return false;
        } catch (const SemanticError& e) {
            return true;
        }
    }

    // Helper to check if analysis of a file throws an error
    bool hasErrorFile(const std::string& filename) {
        try {
            analyzeFile(filename);
            return false;
        } catch (const SemanticError& e) {
            return true;
        }
    }

    TypeContext typeCtx_;
};

// Test basic integer literal
TEST_F(SemanticTest, IntegerLiteral) {
    TypePtr type = analyze("42");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test string literal
TEST_F(SemanticTest, StringLiteral) {
    TypePtr type = analyze("\"hello\"");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isString());
}

// Test nil literal
TEST_F(SemanticTest, NilLiteral) {
    TypePtr type = analyze("nil");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isNil());
}

// Test simple arithmetic
TEST_F(SemanticTest, SimpleArithmetic) {
    TypePtr type = analyze("3 + 4");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test variable declaration and usage
TEST_F(SemanticTest, VariableDeclaration) {
    TypePtr type = analyze("let var x := 5 in x end");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test variable type annotation
TEST_F(SemanticTest, VariableTypeAnnotation) {
    TypePtr type = analyze("let var x : int := 5 in x end");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test undefined variable should error
TEST_F(SemanticTest, UndefinedVariable) { EXPECT_TRUE(hasError("let var x := 5 in y end")); }

// Test if expression
TEST_F(SemanticTest, IfExpression) {
    TypePtr type = analyze("if 1 then 2 else 3");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test while loop
TEST_F(SemanticTest, WhileLoop) {
    TypePtr type = analyze("while 1 do 1");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isVoid());
}

// Test for loop
TEST_F(SemanticTest, ForLoop) {
    TypePtr type = analyze("for i := 1 to 10 do i");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isVoid());
}

// Test break in loop
TEST_F(SemanticTest, BreakInLoop) {
    TypePtr type = analyze("while 1 do break");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isVoid());
}

// Test break outside loop should error
TEST_F(SemanticTest, BreakOutsideLoop) { EXPECT_TRUE(hasError("break")); }

// Test function declaration and call
TEST_F(SemanticTest, FunctionDeclaration) {
    TypePtr type = analyze("let function f(x: int): int = x + 1 in f(5) end");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test type mismatch in arithmetic
TEST_F(SemanticTest, TypeMismatchInArithmetic) { EXPECT_TRUE(hasError("\"string\" + 5")); }

// Test comparison operations
TEST_F(SemanticTest, ComparisonOperations) {
    TypePtr type = analyze("5 < 10");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test logical operations
TEST_F(SemanticTest, LogicalOperations) {
    TypePtr type = analyze("1 & 0");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test record type declaration
TEST_F(SemanticTest, RecordTypeDeclaration) {
    TypePtr type = analyze("let type point = {x: int, y: int} in nil end");
    ASSERT_NE(type, nullptr);
}

// Test record creation
TEST_F(SemanticTest, RecordCreation) {
    TypePtr type = analyze("let type point = {x: int, y: int} in point{x=1, y=2} end");
    ASSERT_NE(type, nullptr);
}

// Test array type declaration
TEST_F(SemanticTest, ArrayTypeDeclaration) {
    TypePtr type = analyze("let type intArray = array of int in nil end");
    ASSERT_NE(type, nullptr);
}

// Test examples/test1.tig - basic array type
TEST_F(SemanticTest, Test1_ArrayType) {
    TypePtr type = analyzeFile("examples/test1.tig");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->actual()->isArray());
}

// Test examples/test2.tig - type alias
TEST_F(SemanticTest, Test2_TypeAlias) {
    TypePtr type = analyzeFile("examples/test2.tig");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->actual()->isArray());
}

// Test examples/test3.tig - record type
TEST_F(SemanticTest, Test3_RecordType) {
    TypePtr type = analyzeFile("examples/test3.tig");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->actual()->isRecord());
}

// Test examples/test5.tig - recursive types
TEST_F(SemanticTest, Test5_RecursiveTypes) {
    TypePtr type = analyzeFile("examples/test5.tig");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->actual()->isRecord());
}

// Test examples/test16.tig - non-productive type cycle (should fail)
TEST_F(SemanticTest, Test16_NonProductiveTypeCycle) {
    EXPECT_TRUE(hasErrorFile("examples/test16.tig"));
}

// Test examples/test17.tig - interrupted type declarations (should fail)
TEST_F(SemanticTest, Test17_InterruptedTypeDeclarations) {
    EXPECT_TRUE(hasErrorFile("examples/test17.tig"));
}

// Test array creation
TEST_F(SemanticTest, ArrayCreation) {
    TypePtr type = analyze("let type intArray = array of int in intArray[10] of 0 end");
    ASSERT_NE(type, nullptr);
}

// Test array subscript
TEST_F(SemanticTest, ArraySubscript) {
    // TODO: Fix test - complex variable access syntax
    // TypePtr type = analyze("let type intArray = array of int var a := intArray[10] of 0 in a[5]
    // end"); ASSERT_NE(type, nullptr); EXPECT_TRUE(type->isInt());
}

// Test array index must be integer
TEST_F(SemanticTest, ArrayIndexMustBeInteger) {
    // TODO: Fix test
    // EXPECT_TRUE(hasError("let type intArray = array of int var a := intArray[10] of 0 in
    // a[\"string\"] end"));
}

// Test field access
TEST_F(SemanticTest, FieldAccess) {
    // TODO: Fix test - record field access syntax
    // TypePtr type = analyze("let type point = {x: int, y: int} in point{x=1, y=2}.x end");
    // ASSERT_NE(type, nullptr);
    // EXPECT_TRUE(type->isInt());
}

// Test assignment
TEST_F(SemanticTest, Assignment) {
    TypePtr type = analyze("let var x := 5 in x := 10 end");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isVoid());
}

// Test assignment type mismatch
TEST_F(SemanticTest, AssignmentTypeMismatch) {
    EXPECT_TRUE(hasError("let var x : int := 5 in x := \"string\" end"));
}

// Test function with wrong argument count
TEST_F(SemanticTest, FunctionWrongArgumentCount) {
    EXPECT_TRUE(hasError("let function f(x: int): int = x + 1 in f() end"));
}

// Test function with wrong argument type
TEST_F(SemanticTest, FunctionWrongArgumentType) {
    EXPECT_TRUE(hasError("let function f(x: int): int = x + 1 in f(\"string\") end"));
}

// Test nested scopes
TEST_F(SemanticTest, NestedScopes) {
    TypePtr type = analyze("let var x := 1 in let var x := 2 in x end end");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test sequence expression
TEST_F(SemanticTest, SequenceExpression) {
    TypePtr type = analyze("(1; 2; 3)");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test void return from function
TEST_F(SemanticTest, VoidReturnFromFunction) {
    TypePtr type = analyze("let function f() = () in f() end");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isVoid());
}

// Test string comparison
TEST_F(SemanticTest, StringComparison) {
    TypePtr type = analyze("\"a\" = \"b\"");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->isInt());
}

// Test nil assignment to record
TEST_F(SemanticTest, NilAssignmentToRecord) {
    TypePtr type =
        analyze("let type point = {x: int, y: int} var p := point{x=1, y=2} in p := nil end");
    ASSERT_NE(type, nullptr);
}

// Test cannot assign to for loop variable
TEST_F(SemanticTest, CannotAssignToForLoopVariable) {
    EXPECT_TRUE(hasError("for i := 1 to 10 do i := 5"));
}

// Test recursive record type
TEST_F(SemanticTest, RecursiveRecordType) {
    TypePtr type = analyze(
        "let "
        "  type intlist = {head: int, tail: intlist} "
        "  var l := intlist{head=1, tail=nil}"
        "in "
        "  l "
        "end");
    ASSERT_NE(type, nullptr);

    EXPECT_TRUE(type->actual()->isRecord());

    auto record = dynamic_cast<semantic::RecordType*>(type->actual());
    ASSERT_NE(record, nullptr);
    const auto& fields = record->getFields();
    ASSERT_EQ(fields.size(), 2);
    EXPECT_EQ(fields[0].name, "head");
    EXPECT_TRUE(fields[0].type->actual()->isInt());
    EXPECT_EQ(fields[1].name, "tail");
    EXPECT_EQ(fields[1].type->actual(), record);
}

// Test mutually recursive types
TEST_F(SemanticTest, MutuallyRecursiveTypes) {
    TypePtr type = analyze(
        "let "
        "  type tree = {key: int, children: treelist} "
        "  type treelist = {head: tree, tail: treelist} "
        "  var t := tree{key=0, children=nil} "
        "in "
        "  t "
        "end");
    ASSERT_NE(type, nullptr);
    EXPECT_TRUE(type->actual()->isRecord());

    auto treeRecord = dynamic_cast<semantic::RecordType*>(type->actual());
    ASSERT_NE(treeRecord, nullptr);

    auto childrenFieldType = treeRecord->getFieldType("children");
    ASSERT_NE(childrenFieldType, nullptr);
    EXPECT_TRUE(childrenFieldType->actual()->isRecord());

    auto treelistRecord = dynamic_cast<semantic::RecordType*>(childrenFieldType->actual());
    ASSERT_NE(treelistRecord, nullptr);

    auto headFieldType = treelistRecord->getFieldType("head");
    ASSERT_NE(headFieldType, nullptr);
    EXPECT_EQ(headFieldType->actual(), treeRecord);
}

}  // namespace test
}  // namespace tiger
