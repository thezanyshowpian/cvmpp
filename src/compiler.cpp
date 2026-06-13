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

size_t Compiler::emitJump(OpCode op, int line) {
    chunk_.writeOpCode(op, line);
    chunk_.writeByte(0xFF, line);
    chunk_.writeByte(0xFF, line);
    return chunk_.code.size() - 2;
}

void Compiler::patchJump(size_t jumpOffset, int line) {
    size_t distance = chunk_.code.size() - jumpOffset - 2;
    if (distance > static_cast<size_t>(UINT16_MAX)) {
        error(line, "jump distance exceeds 65535 bytes");
        return;
    }
    auto d = static_cast<uint16_t>(distance);
    chunk_.code[jumpOffset]     = static_cast<uint8_t>((d >> 8) & 0xFF);
    chunk_.code[jumpOffset + 1] = static_cast<uint8_t>(d & 0xFF);
}

void Compiler::emitLoop(size_t loopStart, int line) {
    chunk_.writeOpCode(OpCode::OP_LOOP, line);
    size_t offset = chunk_.code.size() + 2 - loopStart;
    if (offset > static_cast<size_t>(UINT16_MAX)) {
        error(line, "loop body exceeds 65535 bytes");
        return;
    }
    chunk_.writeShort(static_cast<uint16_t>(offset), line);
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
    compileExpr(*s.condition);
    size_t thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE, s.line);
    chunk_.writeOpCode(OpCode::OP_POP, s.line);    // pop condition — true path

    s.thenBranch->accept(*this);

    size_t elseJump = emitJump(OpCode::OP_JUMP, s.line);  // skip false-path pop (+ else)
    patchJump(thenJump, s.line);
    chunk_.writeOpCode(OpCode::OP_POP, s.line);    // pop condition — false path

    if (s.elseBranch != nullptr)
        s.elseBranch->accept(*this);
    patchJump(elseJump, s.line);
}

void Compiler::visitWhileStmt(WhileStmt& s) {
    size_t loopStart = chunk_.code.size();

    compileExpr(*s.condition);
    size_t exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, s.line);
    chunk_.writeOpCode(OpCode::OP_POP, s.line);    // pop condition — true path

    s.body->accept(*this);
    emitLoop(loopStart, s.line);

    patchJump(exitJump, s.line);
    chunk_.writeOpCode(OpCode::OP_POP, s.line);    // pop condition — false path
}

void Compiler::visitBlockStmt(BlockStmt& s) {
    for (const auto& stmt : s.statements)
        stmt->accept(*this);
}
