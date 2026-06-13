#include "cvm/compiler.hpp"
#include <iostream>

void Compiler::error(int line, const std::string& msg) {
    std::cerr << "[line " << line << "] Compile error: " << msg << '\n';
    hadError_ = true;
}

void Compiler::compileExpr(Expr& e) {
    e.accept(*this);
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
    error(e.line, "variables not yet implemented (phase 6)");
}

void Compiler::visitAssignExpr(Assign& e) {
    error(e.line, "assignment not yet implemented (phase 6)");
}

void Compiler::visitInputExpr(Input& e) {
    error(e.line, "input not yet implemented (phase 6)");
}

// ── StmtVisitor ───────────────────────────────────────────────────────────────

void Compiler::visitExprStmt(ExprStmt& s) {
    compileExpr(*s.expression);
    chunk_.writeOpCode(OpCode::OP_POP, s.line);
}

void Compiler::visitLetStmt(LetStmt& s) {
    error(s.line, "let statement not yet implemented (phase 6)");
}

void Compiler::visitPrintStmt(PrintStmt& s) {
    error(s.line, "print statement not yet implemented (phase 5)");
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
