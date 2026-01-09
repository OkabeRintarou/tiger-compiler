#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ast/ast.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "translate/escape.hpp"

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);

        // Lexical analysis
        tiger::Lexer lexer(source);
        std::vector<tiger::Token> tokens = lexer.tokenize();
        std::cout << "Lexical analysis completed: " << tokens.size() << " tokens" << std::endl;

        // Parsing
        tiger::Parser parser(tokens);
        tiger::ast::ExprPtr ast = parser.parse();
        std::cout << "Parsing completed successfully" << std::endl;

        // Escape analysis (before semantic analysis)
        tiger::translate::findEscapes(ast);
        std::cout << "Escape analysis completed" << std::endl;

        // Semantic analysis
        tiger::semantic::TypeContext typeCtx;
        tiger::semantic::SemanticAnalyzer semantic(typeCtx);
        auto resultType = semantic.analyze(ast);
        std::cout << "Semantic analysis completed successfully" << std::endl;

        std::cout << "\nCompilation completed successfully" << std::endl;
        return 0;

    } catch (const tiger::semantic::SemanticError& e) {
        std::cerr << "Semantic error at (" << e.line() << "," << e.column() << "): " << e.what()
                  << std::endl;
        return 1;
    } catch (const tiger::SyntaxError& e) {
        std::cerr << "Syntax error" << std::endl;
        return 1;
    } catch (const tiger::TigerError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}
