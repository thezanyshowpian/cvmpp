#include "cvm/compiler.hpp"
#include <iostream>

void Compiler::error(int line, const std::string& msg) {
    std::cerr << "[line " << line << "] Compile error: " << msg << '\n';
    hadError_ = true;
}

void Compiler::compileExpr(Expr& e) {
    e.accept(*this);
}

int Compiler::identifierIndex(const std::string& name) {
    for (int i = 0; i < static_cast<int>(chunk_.identifiers.size()); ++i)
        if (chunk_.identifiers[i] == name) return i;
    return chunk_.addIdentifier(name);
}

Chunk Compiler::compile(const std::vector<StmtPtr>& stmts) {
    chunk_    = Chunk{};
    hadError_ = false;
    for (const auto& s : stmts)
        s->accept(*this);
    int lastLine = stmts.empty() ? 1 : stmts.back()->line;
    chunk_.writeOpCode(OpCode::OP_HALT, lastLine);
    return std::move(chunk_);
}

// ── ExprVisitor ───────────────────────────────────────────────────────────────

void Compiler::visitLiteralExpr(Literal& e) {
    if (e.value.type == ValueType::BOOL) {
        chunk_.writeOpCode(e.value.b ? OpCode::OP_TRUE : OpCode::OP_FALSE, e.line);
    } else {
        int idx = chunk_.addConstant(e.value);
        chunk_.writeOpCode(OpCode::OP_CONSTANT, e.line);
        chunk_.writeByte(static_cast<uint8_t>(idx), e.line);
    }
}

void Compiler::visitUnaryExpr(Unary& e) {
    compileExpr(*e.right);
    chunk_.writeOpCode(OpCode::OP_NEGATE, e.line);
}

void Compiler::visitBinaryExpr(Binary& e) {
    compileExpr(*e.left);
    compileExpr(*e.right);
    switch (e.op.type) {
        case TokenType::PLUS:        chunk_.writeOpCode(OpCode::OP_ADD,   e.line); break;
        case TokenType::MINUS:       chunk_.writeOpCode(OpCode::OP_SUB,   e.line); break;
        case TokenType::STAR:        chunk_.writeOpCode(OpCode::OP_MUL,   e.line); break;
        case TokenType::SLASH:       chunk_.writeOpCode(OpCode::OP_DIV,   e.line); break;
        case TokenType::EQUAL_EQUAL: chunk_.writeOpCode(OpCode::OP_EQUAL, e.line); break;
        case TokenType::LESS:        chunk_.writeOpCode(OpCode::OP_LESS,  e.line); break;
        default:
            error(e.line, "unknown binary operator '" + e.op.lexeme + "'");
    }
}

void Compiler::visitVariableExpr(Variable& e) {
    int idx = identifierIndex(e.name.lexeme);
    chunk_.writeOpCode(OpCode::OP_GET_GLOBAL, e.line);
    chunk_.writeByte(static_cast<uint8_t>(idx), e.line);
}

void Compiler::visitAssignExpr(Assign& e) {
    compileExpr(*e.value);
    int idx = identifierIndex(e.name.lexeme);
    chunk_.writeOpCode(OpCode::OP_SET_GLOBAL, e.line);
    chunk_.writeByte(static_cast<uint8_t>(idx), e.line);
}

void Compiler::visitInputExpr(Input& e) {
    chunk_.writeOpCode(OpCode::OP_INPUT, e.line);
}

// ── StmtVisitor ───────────────────────────────────────────────────────────────

void Compiler::visitExprStmt(ExprStmt& s) {
    compileExpr(*s.expression);
    chunk_.writeOpCode(OpCode::OP_POP, s.line);
}

void Compiler::visitLetStmt(LetStmt& s) {
    compileExpr(*s.initializer);
    int idx = identifierIndex(s.name.lexeme);
    chunk_.writeOpCode(OpCode::OP_DEFINE_GLOBAL, s.line);
    chunk_.writeByte(static_cast<uint8_t>(idx), s.line);
}

void Compiler::visitPrintStmt(PrintStmt& s) {
    compileExpr(*s.expression);
    chunk_.writeOpCode(OpCode::OP_PRINT, s.line);
}

void Compiler::visitIfStmt(IfStmt& s) {
    error(s.line, "if statement not yet implemented (phase 7)");
}

void Compiler::visitWhileStmt(WhileStmt& s) {
    error(s.line, "while statement not yet implemented (phase 7)");
}

void Compiler::visitBlockStmt(BlockStmt& s) {
    error(s.line, "block statement not yet implemented (phase 7)");
}
