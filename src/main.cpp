#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ast/ast.hpp"
#include "ir/ir_generator.hpp"
#include "ir/print.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "translate/escape.hpp"
#include "translate/x64_frame.hpp"

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
        std::cout << "Usage: " << argv[0] << " <input_file> [--dump-ir]" << std::endl;
        return 1;
    }

    bool dumpIR = false;
    std::string inputFile = argv[1];

    // Parse command line options
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--dump-ir") {
            dumpIR = true;
        }
    }

    try {
        std::string source = readFile(inputFile);

        // Lexical analysis
        tiger::Lexer lexer(source);
        std::vector<tiger::Token> tokens = lexer.tokenize();
        std::cout << "Lexical analysis completed: " << tokens.size() << " tokens" << std::endl;

        // Parsing
        tiger::Parser parser(tokens);
        tiger::ast::ExprPtr ast = parser.parse();
        std::cout << "Parsing completed successfully" << std::endl;

        // Escape analysis
        tiger::translate::findEscapes(ast);
        std::cout << "Escape analysis completed" << std::endl;

        // Semantic analysis
        tiger::semantic::TypeContext typeCtx;
        tiger::semantic::SemanticAnalyzer semantic(typeCtx);
        auto resultType = semantic.analyze(ast);
        std::cout << "Semantic analysis completed successfully" << std::endl;

        // IR generation
        auto frameFactory = std::make_shared<tiger::translate::X64FrameFactory>();
        tiger::ir::IRGenerator irGen(frameFactory);
        irGen.generate(ast.get());
        std::cout << "IR generation completed: " << irGen.fragments().size() << " fragments"
                  << std::endl;

        // Optionally dump IR
        if (dumpIR) {
            std::cout << "\n========== IR Dump ==========\n" << std::endl;
            tiger::ir::TreePrinter printer(std::cout);

            for (size_t i = 0; i < irGen.fragments().size(); ++i) {
                auto& frag = irGen.fragments()[i];

                // Check if it's a ProcFragment
                if (auto procFrag = dynamic_cast<tiger::ir::ProcFragment*>(frag.get())) {
                    std::cout << "Fragment #" << i
                              << " (Procedure): " << procFrag->frame()->name().name() << "\n";
                    printer.print(procFrag->body());
                    std::cout << "\n";
                }
                // Check if it's a StringFragment
                else if (auto strFrag = dynamic_cast<tiger::ir::StringFragment*>(frag.get())) {
                    std::cout << "Fragment #" << i << " (String): " << strFrag->label().name()
                              << " = \"" << strFrag->value() << "\"\n\n";
                }
            }
            std::cout << "========== End IR Dump ==========\n" << std::endl;
        }

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
