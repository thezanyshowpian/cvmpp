#pragma once
#include <string>
#include <vector>
#include "cvm/ast.hpp"
#include "cvm/chunk.hpp"

// AstPrinter: walks the AST and emits a parenthesised S-expression to stdout,
// one line per top-level statement.  Used by --ast in the CLI.
class AstPrinter : public ExprVisitor, public StmtVisitor {
public:
    void print(const std::vector<StmtPtr>& stmts);

private:
    std::string result_;   // accumulates the current node's text

    std::string printExpr(Expr& e);
    std::string printStmtStr(Stmt& s);

    // ExprVisitor
    void visitBinaryExpr(Binary& e)   override;
    void visitUnaryExpr(Unary& e)     override;
    void visitLiteralExpr(Literal& e) override;
    void visitVariableExpr(Variable& e) override;
    void visitAssignExpr(Assign& e)   override;
    void visitInputExpr(Input& e)     override;

    // StmtVisitor
    void visitLetStmt(LetStmt& s)     override;
    void visitPrintStmt(PrintStmt& s) override;
    void visitIfStmt(IfStmt& s)       override;
    void visitWhileStmt(WhileStmt& s) override;
    void visitBlockStmt(BlockStmt& s) override;
    void visitExprStmt(ExprStmt& s)   override;
};

// BytecodeDisassembler: walks a Chunk and prints aligned, readable instructions.
// Output format (per instruction):
//   OOOO LLLL NAME                 [operand info]
// where OOOO is the byte offset, LLLL is the source line (or '|' for same line).
class BytecodeDisassembler {
public:
    explicit BytecodeDisassembler(const Chunk& chunk, std::string name = "");
    void disassemble() const;

private:
    const Chunk& chunk_;
    std::string  name_;

    size_t disassembleInstruction(size_t offset) const;
    size_t simpleInstruction(const char* name, size_t offset) const;
    size_t constantInstruction(const char* name, size_t offset) const;
    size_t byteInstruction(const char* name, size_t offset) const;
    size_t jumpInstruction(const char* name, int sign, size_t offset) const;
};
