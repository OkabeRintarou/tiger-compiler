#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

namespace tiger {
namespace test {

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

class ParserTest : public ::testing::Test {
   protected:
    bool parseFile(const std::string& filename) {
        try {
            std::string source = readFile(filename);
            Lexer lexer(source);
            std::vector<Token> tokens = lexer.tokenize();
            Parser parser(tokens);
            ExprPtr ast = parser.parse();
            return true;
        } catch (const SyntaxError& e) {
            return false;
        } catch (const std::exception& e) {
            return false;
        }
    }
};

TEST_F(ParserTest, Test1_NestedComments) { EXPECT_TRUE(parseFile("examples/test1.tig")); }

TEST_F(ParserTest, Test2_TypeAliases) { EXPECT_TRUE(parseFile("examples/test2.tig")); }

TEST_F(ParserTest, Test3_BasicExpressions) { EXPECT_TRUE(parseFile("examples/test3.tig")); }

TEST_F(ParserTest, Test4_ArrayOperations) { EXPECT_TRUE(parseFile("examples/test4.tig")); }

TEST_F(ParserTest, Test5_RecordOperations) { EXPECT_TRUE(parseFile("examples/test5.tig")); }

TEST_F(ParserTest, Test6_Through_Test10) {
    EXPECT_TRUE(parseFile("examples/test6.tig"));
    EXPECT_TRUE(parseFile("examples/test7.tig"));
    EXPECT_TRUE(parseFile("examples/test8.tig"));
    EXPECT_TRUE(parseFile("examples/test9.tig"));
    EXPECT_TRUE(parseFile("examples/test10.tig"));
}

TEST_F(ParserTest, Test11_Through_Test20) {
    for (int i = 11; i <= 20; i++) {
        std::string file = "examples/test" + std::to_string(i) + ".tig";
        EXPECT_TRUE(parseFile(file)) << "Failed to parse " << file;
    }
}

TEST_F(ParserTest, Test21_Through_Test30) {
    for (int i = 21; i <= 30; i++) {
        std::string file = "examples/test" + std::to_string(i) + ".tig";
        EXPECT_TRUE(parseFile(file)) << "Failed to parse " << file;
    }
}

TEST_F(ParserTest, Test31_Through_Test40) {
    for (int i = 31; i <= 40; i++) {
        std::string file = "examples/test" + std::to_string(i) + ".tig";
        EXPECT_TRUE(parseFile(file)) << "Failed to parse " << file;
    }
}

TEST_F(ParserTest, Test41_Through_Test48) {
    for (int i = 41; i <= 48; i++) {
        std::string file = "examples/test" + std::to_string(i) + ".tig";
        EXPECT_TRUE(parseFile(file)) << "Failed to parse " << file;
    }
}

TEST_F(ParserTest, Test49_SyntaxError) { EXPECT_FALSE(parseFile("examples/test49.tig")); }

TEST_F(ParserTest, QueensProgram) { EXPECT_TRUE(parseFile("examples/queens.tig")); }

TEST_F(ParserTest, MergeSortProgram) { EXPECT_TRUE(parseFile("examples/merge.tig")); }

}  // namespace test
}  // namespace tiger

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
