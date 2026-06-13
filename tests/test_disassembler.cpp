#include "cvm/chunk.hpp"
#include "cvm/disassembler.hpp"
#include <iostream>
#include <sstream>
#include <string>

static int g_passed = 0;
static int g_failed = 0;

static void check(bool cond, const char* desc) {
    if (cond) { std::cout << "  PASS: " << desc << '\n'; ++g_passed; }
    else       { std::cout << "  FAIL: " << desc << '\n'; ++g_failed; }
}

// ── Test A: Chunk API ─────────────────────────────────────────────────────────

static void testChunkApi() {
    std::cout << "Test A: Chunk API\n";
    Chunk chunk;

    // addConstant returns sequential indices
    int i0 = chunk.addConstant(Value::makeInt(42));
    int i1 = chunk.addConstant(Value::makeInt(10));
    check(i0 == 0,                          "first addConstant returns 0");
    check(i1 == 1,                          "second addConstant returns 1");
    check(chunk.constants.size() == 2,      "constants pool has 2 entries");

    // addIdentifier returns sequential indices
    int n0 = chunk.addIdentifier("x");
    int n1 = chunk.addIdentifier("y");
    check(n0 == 0, "first addIdentifier returns 0");
    check(n1 == 1, "second addIdentifier returns 1");

    // writeByte adds to code and lines in lockstep
    chunk.writeByte(0xAB, 5);
    check(chunk.code.size()  == 1,          "code has 1 byte after writeByte");
    check(chunk.lines.size() == 1,          "lines has 1 entry after writeByte");
    check(chunk.code[0]  == 0xAB,           "code[0] is 0xAB");
    check(chunk.lines[0] == 5,              "lines[0] is 5");

    // writeOpCode casts OpCode to uint8_t
    chunk.writeOpCode(OpCode::OP_HALT, 6);
    check(chunk.code.size() == 2,           "code has 2 bytes after writeOpCode");
    check(chunk.code[1] == static_cast<uint8_t>(OpCode::OP_HALT),
                                            "code[1] encodes OP_HALT");

    // writeShort is big-endian
    Chunk c2;
    c2.writeShort(0x0102, 1);
    check(c2.code.size() == 2,              "writeShort writes 2 bytes");
    check(c2.code[0] == 0x01,              "writeShort hi byte is 0x01");
    check(c2.code[1] == 0x02,              "writeShort lo byte is 0x02");
    check(c2.lines[0] == 1 && c2.lines[1] == 1,
                                            "writeShort records line for both bytes");
}

// ── Test B: Disassembler output ───────────────────────────────────────────────

static void testDisassemblerOutput() {
    std::cout << "\nTest B: Disassembler output\n";

    // Hand-build a chunk representing: print 42 + 10;  (line 1), OP_HALT (line 2)
    Chunk chunk;
    int c0 = chunk.addConstant(Value::makeInt(42));
    int c1 = chunk.addConstant(Value::makeInt(10));
    chunk.writeOpCode(OpCode::OP_CONSTANT, 1);
    chunk.writeByte(static_cast<uint8_t>(c0), 1);
    chunk.writeOpCode(OpCode::OP_CONSTANT, 1);
    chunk.writeByte(static_cast<uint8_t>(c1), 1);
    chunk.writeOpCode(OpCode::OP_ADD,   1);
    chunk.writeOpCode(OpCode::OP_PRINT, 1);
    chunk.writeOpCode(OpCode::OP_HALT,  2);

    // Capture stdout
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    BytecodeDisassembler dis(chunk, "print 42+10");
    dis.disassemble();
    std::cout.rdbuf(old);
    std::string got = oss.str();

    // Print what we got so it's visible when running with --output-on-failure
    std::cout << got;

    const std::string expected =
        "== print 42+10 ==\n"
        "0000    1 OP_CONSTANT            0 '42'\n"
        "0002    | OP_CONSTANT            1 '10'\n"
        "0004    | OP_ADD\n"
        "0005    | OP_PRINT\n"
        "0006    2 OP_HALT\n";

    if (got == expected) {
        std::cout << "  PASS: disassembly output matches expected\n";
        ++g_passed;
    } else {
        std::cout << "  FAIL: disassembly output mismatch\n";
        std::cout << "--- expected ---\n" << expected << "--- got ---\n" << got;
        ++g_failed;
    }
}

// ── Test C: Global instruction disassembly ────────────────────────────────────

static void testGlobalInstructions() {
    std::cout << "\nTest C: Global instruction disassembly\n";

    Chunk chunk;
    int n = chunk.addIdentifier("counter");
    chunk.addConstant(Value::makeInt(0));
    chunk.writeOpCode(OpCode::OP_CONSTANT,      1); chunk.writeByte(0, 1);
    chunk.writeOpCode(OpCode::OP_DEFINE_GLOBAL, 1);
    chunk.writeByte(static_cast<uint8_t>(n), 1);
    chunk.writeOpCode(OpCode::OP_GET_GLOBAL,    2);
    chunk.writeByte(static_cast<uint8_t>(n), 2);
    chunk.writeOpCode(OpCode::OP_HALT, 2);

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    BytecodeDisassembler dis(chunk, "globals");
    dis.disassemble();
    std::cout.rdbuf(old);
    std::string got = oss.str();
    std::cout << got;

    check(got.find("OP_DEFINE_GLOBAL") != std::string::npos &&
          got.find("'counter'")        != std::string::npos,
          "OP_DEFINE_GLOBAL shows identifier name");
    check(got.find("OP_GET_GLOBAL")    != std::string::npos,
          "OP_GET_GLOBAL is present");
}

// ── Test D: Jump instruction disassembly ──────────────────────────────────────

static void testJumpInstructions() {
    std::cout << "\nTest D: Jump instruction disassembly\n";

    // OP_JUMP 0x0003  (forward by 3 bytes → target = 0 + 3 + 3 = 6)
    Chunk chunk;
    chunk.writeOpCode(OpCode::OP_JUMP, 1);
    chunk.writeShort(3, 1);
    chunk.writeOpCode(OpCode::OP_HALT, 1);

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    BytecodeDisassembler dis(chunk, "jump");
    dis.disassemble();
    std::cout.rdbuf(old);
    std::string got = oss.str();
    std::cout << got;

    check(got.find("OP_JUMP") != std::string::npos, "OP_JUMP is present");
    check(got.find("-> 0006") != std::string::npos, "jump target is 0006");
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    testChunkApi();
    testDisassemblerOutput();
    testGlobalInstructions();
    testJumpInstructions();
    std::cout << '\n' << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
