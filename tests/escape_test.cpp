#include "translate/escape.hpp"

#include <gtest/gtest.h>

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

namespace tiger {
namespace test {

using namespace tiger::ast;
using namespace tiger::translate;

class EscapeTest : public ::testing::Test {
protected:
    ExprPtr parse(const std::string& source) {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        Parser parser(tokens);
        return parser.parse();
    }

    void analyzeEscape(ExprPtr& ast) { findEscapes(ast); }

    VarDecl* findVarDecl(LetExpr* let, const std::string& name) {
        for (auto& decl : let->decls) {
            if (auto* varDecl = dynamic_cast<VarDecl*>(decl.get())) {
                if (varDecl->name == name) return varDecl;
            }
        }
        return nullptr;
    }

    FunctionDecl* findFuncDecl(LetExpr* let, const std::string& name) {
        for (auto& decl : let->decls) {
            if (auto* funcDecl = dynamic_cast<FunctionDecl*>(decl.get())) {
                if (funcDecl->name == name) return funcDecl;
            }
        }
        return nullptr;
    }
};

// Simple variable that doesn't escape
TEST_F(EscapeTest, SimpleVarNoEscape) {
    auto ast = parse("let var x := 5 in x end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* varDecl = findVarDecl(let, "x");
    ASSERT_NE(varDecl, nullptr);
    EXPECT_FALSE(varDecl->escape);
}

// Variable used in nested function escapes
TEST_F(EscapeTest, VarUsedInNestedFunctionEscapes) {
    auto ast = parse(
        "let "
        "  var x := 5 "
        "  function f() : int = x "
        "in f() end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* varDecl = findVarDecl(let, "x");
    ASSERT_NE(varDecl, nullptr);
    EXPECT_TRUE(varDecl->escape);
}

// Variable used two levels deep escapes
TEST_F(EscapeTest, VarUsedTwoLevelsDeepEscapes) {
    auto ast = parse(
        "let "
        "  var x := 5 "
        "  function outer() : int = "
        "    let "
        "      function inner() : int = x "
        "    in inner() end "
        "in outer() end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* xDecl = findVarDecl(let, "x");
    ASSERT_NE(xDecl, nullptr);
    EXPECT_TRUE(xDecl->escape);
}

// Function parameter escapes when used in nested function
TEST_F(EscapeTest, FunctionParamEscapes) {
    auto ast = parse(
        "let "
        "  function outer(n: int) : int = "
        "    let "
        "      function inner() : int = n "
        "    in inner() end "
        "in outer(5) end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* outerDecl = findFuncDecl(let, "outer");
    ASSERT_NE(outerDecl, nullptr);
    ASSERT_EQ(outerDecl->params.size(), 1);
    EXPECT_TRUE(outerDecl->params[0]->escape);
}

// Function parameter doesn't escape when only used locally
TEST_F(EscapeTest, FunctionParamNoEscape) {
    auto ast = parse(
        "let "
        "  function f(n: int) : int = n + 1 "
        "in f(5) end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* fDecl = findFuncDecl(let, "f");
    ASSERT_NE(fDecl, nullptr);
    ASSERT_EQ(fDecl->params.size(), 1);
    EXPECT_FALSE(fDecl->params[0]->escape);
}

// For loop variable doesn't escape
TEST_F(EscapeTest, ForLoopVarNoEscape) {
    auto ast = parse("for i := 0 to 10 do (i; ())");
    analyzeEscape(ast);
    auto* forExpr = dynamic_cast<ForExpr*>(ast.get());
    ASSERT_NE(forExpr, nullptr);
    EXPECT_FALSE(forExpr->escape);
}

// Multiple variables, some escape some don't
TEST_F(EscapeTest, MixedEscapeStatus) {
    auto ast = parse(
        "let "
        "  var a := 1 "
        "  var b := 2 "
        "  var c := 3 "
        "  function f() : int = a + c "
        "in b end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* aDecl = findVarDecl(let, "a");
    auto* bDecl = findVarDecl(let, "b");
    auto* cDecl = findVarDecl(let, "c");
    ASSERT_NE(aDecl, nullptr);
    ASSERT_NE(bDecl, nullptr);
    ASSERT_NE(cDecl, nullptr);
    EXPECT_TRUE(aDecl->escape);
    EXPECT_FALSE(bDecl->escape);
    EXPECT_TRUE(cDecl->escape);
}

// Variable in inner function doesn't escape to outer
TEST_F(EscapeTest, InnerVarNoEscapeToOuter) {
    auto ast = parse(
        "let "
        "  function outer() : int = "
        "    let var local := 10 in local end "
        "in outer() end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* outerDecl = findFuncDecl(let, "outer");
    ASSERT_NE(outerDecl, nullptr);
    auto* innerLet = dynamic_cast<LetExpr*>(outerDecl->body.get());
    ASSERT_NE(innerLet, nullptr);
    auto* localDecl = findVarDecl(innerLet, "local");
    ASSERT_NE(localDecl, nullptr);
    EXPECT_FALSE(localDecl->escape);
}

// Assignment to variable in nested function
TEST_F(EscapeTest, AssignmentInNestedFunction) {
    auto ast = parse(
        "let "
        "  var x := 0 "
        "  function inc() = x := x + 1 "
        "in inc() end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* xDecl = findVarDecl(let, "x");
    ASSERT_NE(xDecl, nullptr);
    EXPECT_TRUE(xDecl->escape);
}

// Array subscript access in nested function
TEST_F(EscapeTest, ArraySubscriptAccess) {
    auto ast = parse(
        "let "
        "  type intArray = array of int "
        "  var arr := intArray[10] of 0 "
        "  function f() : int = arr[0] "
        "in f() end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* arrDecl = findVarDecl(let, "arr");
    ASSERT_NE(arrDecl, nullptr);
    EXPECT_TRUE(arrDecl->escape);
}

// Record field access in nested function
TEST_F(EscapeTest, RecordFieldAccess) {
    auto ast = parse(
        "let "
        "  type point = {x: int, y: int} "
        "  var p := point{x=1, y=2} "
        "  function getX() : int = p.x "
        "in getX() end");
    analyzeEscape(ast);
    auto* let = dynamic_cast<LetExpr*>(ast.get());
    ASSERT_NE(let, nullptr);
    auto* pDecl = findVarDecl(let, "p");
    ASSERT_NE(pDecl, nullptr);
    EXPECT_TRUE(pDecl->escape);
}

}  // namespace test
}  // namespace tiger
