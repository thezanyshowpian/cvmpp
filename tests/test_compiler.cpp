#include "cvm/chunk.hpp"
#include "cvm/compiler.hpp"
#include "cvm/lexer.hpp"
#include "cvm/opcode.hpp"
#include "cvm/parser.hpp"
#include <iostream>
#include <sstream>
#include <string>

static int g_passed = 0;
static int g_failed = 0;

static void check(bool cond, const char* desc) {
    if (cond) { std::cout << "  PASS: " << desc << '\n'; ++g_passed; }
    else       { std::cout << "  FAIL: " << desc << '\n'; ++g_failed; }
}

static Chunk compile(const std::string& src) {
    Lexer  lexer(src);
    Parser parser(lexer.tokenize());
    auto   stmts = parser.parse();
    Compiler c;
    return c.compile(stmts);
}

static uint8_t op(OpCode o) { return static_cast<uint8_t>(o); }

// ── Test A: integer literal ───────────────────────────────────────────────────

static void testIntLiteral() {
    std::cout << "Test A: integer literal\n";
    Chunk chunk = compile("42;");

    check(chunk.code.size() == 4,                  "code is 4 bytes");
    check(chunk.code[0] == op(OpCode::OP_CONSTANT), "byte 0: OP_CONSTANT");
    check(chunk.code[1] == 0,                       "byte 1: constant index 0");
    check(chunk.code[2] == op(OpCode::OP_POP),      "byte 2: OP_POP");
    check(chunk.code[3] == op(OpCode::OP_HALT),     "byte 3: OP_HALT");

    check(chunk.constants.size() == 1,              "one constant in pool");
    check(chunk.constants[0].type == ValueType::INT, "constant type is INT");
    check(chunk.constants[0].i == 42,               "constant value is 42");
}

// ── Test B: boolean literals ──────────────────────────────────────────────────

static void testBoolLiterals() {
    std::cout << "\nTest B: boolean literals\n";
    {
        Chunk chunk = compile("true;");
        check(chunk.code.size() == 3,                "true: code is 3 bytes");
        check(chunk.code[0] == op(OpCode::OP_TRUE),  "true: byte 0 is OP_TRUE");
        check(chunk.code[1] == op(OpCode::OP_POP),   "true: byte 1 is OP_POP");
        check(chunk.code[2] == op(OpCode::OP_HALT),  "true: byte 2 is OP_HALT");
        check(chunk.constants.empty(),               "true: no constants in pool");
    }
    {
        Chunk chunk = compile("false;");
        check(chunk.code[0] == op(OpCode::OP_FALSE), "false: byte 0 is OP_FALSE");
    }
}

// ── Test C: unary negation ────────────────────────────────────────────────────

static void testUnaryNegate() {
    std::cout << "\nTest C: unary negation\n";
    Chunk chunk = compile("-5;");

    check(chunk.code.size() == 5,                   "code is 5 bytes");
    check(chunk.code[0] == op(OpCode::OP_CONSTANT), "byte 0: OP_CONSTANT");
    check(chunk.code[1] == 0,                        "byte 1: constant index 0");
    check(chunk.code[2] == op(OpCode::OP_NEGATE),   "byte 2: OP_NEGATE");
    check(chunk.code[3] == op(OpCode::OP_POP),      "byte 3: OP_POP");
    check(chunk.code[4] == op(OpCode::OP_HALT),     "byte 4: OP_HALT");
    check(chunk.constants[0].i == 5,                "constant value is 5 (negated at runtime)");
}

// ── Test D: arithmetic precedence — the acceptance check ─────────────────────

static void testArithmeticPrecedence() {
    std::cout << "\nTest D: 1 + 2 * 3  (precedence)\n";
    Chunk chunk = compile("1 + 2 * 3;");

    // Expected: CONSTANT(1), CONSTANT(2), CONSTANT(3), MUL, ADD, POP, HALT
    check(chunk.code.size() == 10, "code is 10 bytes");
    check(chunk.code[0] == op(OpCode::OP_CONSTANT), "byte 0: OP_CONSTANT");
    check(chunk.code[1] == 0,                        "byte 1: idx 0  (value 1)");
    check(chunk.code[2] == op(OpCode::OP_CONSTANT), "byte 2: OP_CONSTANT");
    check(chunk.code[3] == 1,                        "byte 3: idx 1  (value 2)");
    check(chunk.code[4] == op(OpCode::OP_CONSTANT), "byte 4: OP_CONSTANT");
    check(chunk.code[5] == 2,                        "byte 5: idx 2  (value 3)");
    check(chunk.code[6] == op(OpCode::OP_MUL),      "byte 6: OP_MUL   (2*3 first)");
    check(chunk.code[7] == op(OpCode::OP_ADD),      "byte 7: OP_ADD   (then +1)");
    check(chunk.code[8] == op(OpCode::OP_POP),      "byte 8: OP_POP");
    check(chunk.code[9] == op(OpCode::OP_HALT),     "byte 9: OP_HALT");

    check(chunk.constants.size() == 3,              "three constants in pool");
    check(chunk.constants[0].i == 1,                "constants[0] == 1");
    check(chunk.constants[1].i == 2,                "constants[1] == 2");
    check(chunk.constants[2].i == 3,                "constants[2] == 3");
}

// ── Test E: comparison operators ─────────────────────────────────────────────

static void testComparisons() {
    std::cout << "\nTest E: comparison operators\n";
    {
        Chunk chunk = compile("1 < 2;");
        check(chunk.code[4] == op(OpCode::OP_LESS),  "1 < 2 emits OP_LESS");
    }
    {
        Chunk chunk = compile("1 == 1;");
        check(chunk.code[4] == op(OpCode::OP_EQUAL), "1 == 1 emits OP_EQUAL");
    }
}

// ── Test F: subtraction and division ─────────────────────────────────────────

static void testSubDiv() {
    std::cout << "\nTest F: subtraction and division\n";
    {
        Chunk chunk = compile("10 - 3;");
        check(chunk.code[4] == op(OpCode::OP_SUB),   "10 - 3 emits OP_SUB");
    }
    {
        Chunk chunk = compile("8 / 4;");
        check(chunk.code[4] == op(OpCode::OP_DIV),   "8 / 4 emits OP_DIV");
    }
}

// ── Test G: multiple statements ───────────────────────────────────────────────

static void testMultipleStmts() {
    std::cout << "\nTest G: multiple expression statements\n";
    Chunk chunk = compile("1; 2;");
    // CONSTANT 1, POP, CONSTANT 2, POP, HALT
    check(chunk.code.size() == 7,                   "two stmts = 7 bytes");
    check(chunk.code[2] == op(OpCode::OP_POP),      "first stmt ends with POP");
    check(chunk.code[5] == op(OpCode::OP_POP),      "second stmt ends with POP");
    check(chunk.code[6] == op(OpCode::OP_HALT),     "program ends with HALT");
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    testIntLiteral();
    testBoolLiterals();
    testUnaryNegate();
    testArithmeticPrecedence();
    testComparisons();
    testSubDiv();
    testMultipleStmts();
    std::cout << '\n' << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
