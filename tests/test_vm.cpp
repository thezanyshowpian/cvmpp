#include "cvm/compiler.hpp"
#include "cvm/lexer.hpp"
#include "cvm/parser.hpp"
#include "cvm/vm.hpp"
#include <iostream>
#include <sstream>
#include <string>

static int g_passed = 0;
static int g_failed = 0;

static void check(bool cond, const char* desc) {
    if (cond) { std::cout << "  PASS: " << desc << '\n'; ++g_passed; }
    else       { std::cout << "  FAIL: " << desc << '\n'; ++g_failed; }
}

static int runVM(const std::string& src, std::string* out, std::string* err) {
    Lexer  lexer(src);
    Parser parser(lexer.tokenize());
    auto   stmts = parser.parse();
    Compiler c;
    Chunk chunk = c.compile(stmts);

    std::ostringstream oss, ess;
    auto ob = std::cout.rdbuf(oss.rdbuf());
    auto eb = std::cerr.rdbuf(ess.rdbuf());
    VM vm;
    int rc = vm.execute(chunk);
    std::cout.rdbuf(ob);
    std::cerr.rdbuf(eb);

    if (out) *out = oss.str();
    if (err) *err = ess.str();
    return rc;
}

// ── Test A: simple integer print ──────────────────────────────────────────────

static void testIntPrint() {
    std::cout << "Test A: integer print\n";
    std::string out;
    int rc = runVM("print 42;", &out, nullptr);
    check(rc == 0,          "exit code 0");
    check(out == "42\n",    "prints 42");
}

// ── Test B: arithmetic evaluation (precedence end-to-end) ─────────────────────

static void testArithmetic() {
    std::cout << "\nTest B: arithmetic\n";
    std::string out;
    int rc = runVM("print 1 + 2 * 3;", &out, nullptr);
    check(rc == 0,         "exit code 0");
    check(out == "7\n",    "1 + 2*3 = 7");
}

// ── Test C: boolean print ─────────────────────────────────────────────────────

static void testBoolPrint() {
    std::cout << "\nTest C: boolean print\n";
    std::string out;
    int rc;

    rc = runVM("print true;", &out, nullptr);
    check(rc == 0,            "exit code 0 for true");
    check(out == "true\n",    "prints 'true'");

    rc = runVM("print false;", &out, nullptr);
    check(rc == 0,            "exit code 0 for false");
    check(out == "false\n",   "prints 'false'");
}

// ── Test D: type error (1 + true) ────────────────────────────────────────────

static void testTypeError() {
    std::cout << "\nTest D: type error\n";
    std::string err;
    int rc = runVM("print 1 + true;", nullptr, &err);
    check(rc == 70,                          "exit code 70");
    check(err.find("Runtime error") != std::string::npos, "stderr contains 'Runtime error'");
    check(err.find("[line 1]") != std::string::npos,       "stderr contains line number");
}

// ── Test E: division by zero ──────────────────────────────────────────────────

static void testDivisionByZero() {
    std::cout << "\nTest E: division by zero\n";
    std::string err;
    int rc = runVM("print 10 / 0;", nullptr, &err);
    check(rc == 70,                                        "exit code 70");
    check(err.find("division by zero") != std::string::npos, "stderr contains 'division by zero'");
}

// ── Test F: comparison operators ─────────────────────────────────────────────

static void testComparisons() {
    std::cout << "\nTest F: comparisons\n";
    std::string out;
    int rc;

    rc = runVM("print 3 < 5;", &out, nullptr);
    check(rc == 0,           "exit code 0");
    check(out == "true\n",   "3 < 5 is true");

    rc = runVM("print 4 == 4;", &out, nullptr);
    check(rc == 0,           "exit code 0");
    check(out == "true\n",   "4 == 4 is true");

    rc = runVM("print 5 < 3;", &out, nullptr);
    check(rc == 0,           "exit code 0");
    check(out == "false\n",  "5 < 3 is false");
}

// ── Test G: unary negation ────────────────────────────────────────────────────

static void testUnaryNegate() {
    std::cout << "\nTest G: unary negation\n";
    std::string out;
    int rc = runVM("print -5 + 8;", &out, nullptr);
    check(rc == 0,        "exit code 0");
    check(out == "3\n",   "-5 + 8 = 3");
}

// ── Test H: globals ───────────────────────────────────────────────────────────

static void testGlobals() {
    std::cout << "\nTest H: globals\n";
    std::string out;
    int rc = runVM("let x = 10; let y = 20; x = x + y; print x;", &out, nullptr);
    check(rc == 0,        "exit code 0");
    check(out == "30\n",  "prints 30");
}

// ── Test I: undefined variable error ─────────────────────────────────────────

static void testUndefinedVariable() {
    std::cout << "\nTest I: undefined variable\n";
    std::string err;
    int rc = runVM("print z;", nullptr, &err);
    check(rc == 70,                                              "exit code 70");
    check(err.find("Runtime error") != std::string::npos,       "stderr contains 'Runtime error'");
    check(err.find("[line 1]") != std::string::npos,            "stderr contains line number");
    check(err.find("undefined variable") != std::string::npos,  "stderr names the variable");
}

int main() {
    testIntPrint();
    testArithmetic();
    testBoolPrint();
    testTypeError();
    testDivisionByZero();
    testComparisons();
    testUnaryNegate();
    testGlobals();
    testUndefinedVariable();

    std::cout << '\n' << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
