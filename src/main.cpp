#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "cvm/disassembler.hpp"
#include "cvm/lexer.hpp"
#include "cvm/parser.hpp"

static int runFile(const std::string& path, bool printAst) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: cannot open '" << path << "'\n";
        return 1;
    }
    std::ostringstream ss;
    ss << file.rdbuf();

    Lexer lexer(ss.str());
    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    auto stmts = parser.parse();

    if (parser.hadError()) return 65;

    if (printAst) AstPrinter{}.print(stmts);

    return 0;  // execution (VM) wired in Phase 5
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "CVM++ 0.1  --  a tiny statically-typed scripting language\n";
        std::cout << "Usage: cvm <script.cvm> [--ast]\n";
        return 0;
    }

    std::string path;
    bool printAst = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--ast")       { printAst = true; }
        else if (arg[0] == '-')   { std::cerr << "Unknown flag: " << arg << '\n'; return 1; }
        else                      { path = arg; }
    }

    if (path.empty()) {
        std::cerr << "Error: no script file provided.\n";
        return 1;
    }

    return runFile(path, printAst);
}
