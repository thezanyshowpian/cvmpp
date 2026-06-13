#pragma once
#include <string>
#include <vector>
#include "cvm/ast.hpp"
#include "cvm/chunk.hpp"

class Compiler : public ExprVisitor, public StmtVisitor {
public:
    Chunk compile(const std::vector<StmtPtr>& stmts);
    bool  hadError() const { return hadError_; }

private:
    Chunk chunk_;
    bool  hadError_ = false;

    void compileExpr(Expr& e);
    void error(int line, const std::string& msg);

    // ExprVisitor
    void visitLiteralExpr(Literal& e)   override;
    void visitUnaryExpr(Unary& e)       override;
    void visitBinaryExpr(Binary& e)     override;
    void visitVariableExpr(Variable& e) override;
    void visitAssignExpr(Assign& e)     override;
    void visitInputExpr(Input& e)       override;

    // StmtVisitor
    void visitExprStmt(ExprStmt& s)     override;
    void visitLetStmt(LetStmt& s)       override;
    void visitPrintStmt(PrintStmt& s)   override;
    void visitIfStmt(IfStmt& s)         override;
    void visitWhileStmt(WhileStmt& s)   override;
    void visitBlockStmt(BlockStmt& s)   override;
};
