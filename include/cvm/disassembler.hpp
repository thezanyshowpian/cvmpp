#pragma once
#include <string>
#include <vector>
#include "cvm/ast.hpp"

// AstPrinter: walks the AST and emits a parenthesised S-expression to stdout,
// one line per top-level statement.  Used by --ast in the CLI.
// Phase 3 will add a BytecodeDisassembler to this file alongside this class.
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
