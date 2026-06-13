#include "cvm/disassembler.hpp"
#include <iomanip>
#include <iostream>
#include <string>

// ── Helpers ───────────────────────────────────────────────────────────────────

std::string AstPrinter::printExpr(Expr& e) {
    e.accept(*this);
    return result_;
}

std::string AstPrinter::printStmtStr(Stmt& s) {
    s.accept(*this);
    return result_;
}

void AstPrinter::print(const std::vector<StmtPtr>& stmts) {
    for (const auto& s : stmts)
        std::cout << printStmtStr(*s) << '\n';
}

// ── ExprVisitor ───────────────────────────────────────────────────────────────

void AstPrinter::visitBinaryExpr(Binary& e) {
    result_ = "(" + e.op.lexeme
            + " " + printExpr(*e.left)
            + " " + printExpr(*e.right) + ")";
}

void AstPrinter::visitUnaryExpr(Unary& e) {
    result_ = "(" + e.op.lexeme + " " + printExpr(*e.right) + ")";
}

void AstPrinter::visitLiteralExpr(Literal& e) {
    if (e.value.type == ValueType::INT)
        result_ = std::to_string(e.value.i);
    else
        result_ = e.value.b ? "true" : "false";
}

void AstPrinter::visitVariableExpr(Variable& e) {
    result_ = e.name.lexeme;
}

void AstPrinter::visitAssignExpr(Assign& e) {
    result_ = "(assign " + e.name.lexeme + " " + printExpr(*e.value) + ")";
}

void AstPrinter::visitInputExpr(Input&) {
    result_ = "input";
}

// ── StmtVisitor ───────────────────────────────────────────────────────────────

void AstPrinter::visitLetStmt(LetStmt& s) {
    result_ = "(let " + s.name.lexeme + " " + printExpr(*s.initializer) + ")";
}

void AstPrinter::visitPrintStmt(PrintStmt& s) {
    result_ = "(print " + printExpr(*s.expression) + ")";
}

void AstPrinter::visitIfStmt(IfStmt& s) {
    std::string condStr = printExpr(*s.condition);
    std::string thenStr = printStmtStr(*s.thenBranch);
    std::string acc = "(if " + condStr + " " + thenStr;
    if (s.elseBranch)
        acc += " " + printStmtStr(*s.elseBranch);
    acc += ")";
    result_ = acc;
}

void AstPrinter::visitWhileStmt(WhileStmt& s) {
    std::string condStr = printExpr(*s.condition);
    std::string bodyStr = printStmtStr(*s.body);
    result_ = "(while " + condStr + " " + bodyStr + ")";
}

void AstPrinter::visitBlockStmt(BlockStmt& s) {
    std::string acc = "(block";
    for (auto& stmt : s.statements)
        acc += " " + printStmtStr(*stmt);
    acc += ")";
    result_ = acc;
}

void AstPrinter::visitExprStmt(ExprStmt& s) {
    result_ = "(expr-stmt " + printExpr(*s.expression) + ")";
}

// ── BytecodeDisassembler ──────────────────────────────────────────────────────

namespace {
std::string fmtValue(const Value& v) {
    if (v.type == ValueType::INT) return std::to_string(v.i);
    return v.b ? "true" : "false";
}
} // namespace

BytecodeDisassembler::BytecodeDisassembler(const Chunk& chunk, std::string name)
    : chunk_(chunk), name_(std::move(name)) {}

void BytecodeDisassembler::disassemble() const {
    if (!name_.empty())
        std::cout << "== " << name_ << " ==\n";
    size_t offset = 0;
    while (offset < chunk_.code.size())
        offset = disassembleInstruction(offset);
}

size_t BytecodeDisassembler::disassembleInstruction(size_t offset) const {
    // Byte offset: 4-digit zero-padded
    std::cout << std::setfill('0') << std::setw(4) << offset << ' ';
    // Line number: 4-char right-aligned, or "   |" when same as previous byte's line
    std::cout << std::setfill(' ');
    if (offset > 0 && chunk_.lines[offset] == chunk_.lines[offset - 1])
        std::cout << "   | ";
    else
        std::cout << std::setw(4) << chunk_.lines[offset] << ' ';

    auto op = static_cast<OpCode>(chunk_.code[offset]);
    switch (op) {
        case OpCode::OP_CONSTANT:      return constantInstruction("OP_CONSTANT",      offset);
        case OpCode::OP_TRUE:          return simpleInstruction  ("OP_TRUE",           offset);
        case OpCode::OP_FALSE:         return simpleInstruction  ("OP_FALSE",          offset);
        case OpCode::OP_NEGATE:        return simpleInstruction  ("OP_NEGATE",         offset);
        case OpCode::OP_ADD:           return simpleInstruction  ("OP_ADD",            offset);
        case OpCode::OP_SUB:           return simpleInstruction  ("OP_SUB",            offset);
        case OpCode::OP_MUL:           return simpleInstruction  ("OP_MUL",            offset);
        case OpCode::OP_DIV:           return simpleInstruction  ("OP_DIV",            offset);
        case OpCode::OP_EQUAL:         return simpleInstruction  ("OP_EQUAL",          offset);
        case OpCode::OP_LESS:          return simpleInstruction  ("OP_LESS",           offset);
        case OpCode::OP_PRINT:         return simpleInstruction  ("OP_PRINT",          offset);
        case OpCode::OP_POP:           return simpleInstruction  ("OP_POP",            offset);
        case OpCode::OP_DEFINE_GLOBAL: return byteInstruction    ("OP_DEFINE_GLOBAL",  offset);
        case OpCode::OP_GET_GLOBAL:    return byteInstruction    ("OP_GET_GLOBAL",     offset);
        case OpCode::OP_SET_GLOBAL:    return byteInstruction    ("OP_SET_GLOBAL",     offset);
        case OpCode::OP_INPUT:         return simpleInstruction  ("OP_INPUT",          offset);
        case OpCode::OP_JUMP:          return jumpInstruction    ("OP_JUMP",          +1, offset);
        case OpCode::OP_JUMP_IF_FALSE: return jumpInstruction    ("OP_JUMP_IF_FALSE", +1, offset);
        case OpCode::OP_LOOP:          return jumpInstruction    ("OP_LOOP",          -1, offset);
        case OpCode::OP_HALT:          return simpleInstruction  ("OP_HALT",           offset);
        default:
            std::cout << "OP_UNKNOWN ("
                      << static_cast<int>(chunk_.code[offset]) << ")\n";
            return offset + 1;
    }
}

size_t BytecodeDisassembler::simpleInstruction(const char* name, size_t offset) const {
    std::cout << name << '\n';
    return offset + 1;
}

size_t BytecodeDisassembler::constantInstruction(const char* name, size_t offset) const {
    uint8_t idx = chunk_.code[offset + 1];
    std::cout << std::left  << std::setw(20) << name
              << std::right << std::setw(4)  << static_cast<int>(idx);
    if (idx < static_cast<uint8_t>(chunk_.constants.size()))
        std::cout << " '" << fmtValue(chunk_.constants[idx]) << "'";
    std::cout << '\n';
    return offset + 2;
}

size_t BytecodeDisassembler::byteInstruction(const char* name, size_t offset) const {
    uint8_t idx = chunk_.code[offset + 1];
    std::cout << std::left  << std::setw(20) << name
              << std::right << std::setw(4)  << static_cast<int>(idx);
    if (idx < static_cast<uint8_t>(chunk_.identifiers.size()))
        std::cout << " '" << chunk_.identifiers[idx] << "'";
    std::cout << '\n';
    return offset + 2;
}

size_t BytecodeDisassembler::jumpInstruction(const char* name, int sign,
                                             size_t offset) const {
    uint16_t jump = static_cast<uint16_t>(
        (static_cast<int>(chunk_.code[offset + 1]) << 8) |
         static_cast<int>(chunk_.code[offset + 2]));
    auto target = static_cast<size_t>(
        static_cast<long>(offset + 3) + sign * static_cast<long>(jump));
    std::cout << std::left  << std::setw(20) << name
              << std::right << std::setw(4)  << static_cast<int>(jump)
              << " -> " << std::setfill('0') << std::setw(4) << target
              << std::setfill(' ') << '\n';
    return offset + 3;
}
