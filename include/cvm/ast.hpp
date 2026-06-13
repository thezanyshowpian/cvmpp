#pragma once
#include <memory>
#include <vector>
#include "cvm/token.hpp"
#include "cvm/value.hpp"

// Forward declarations for visitor interfaces
struct Binary;
struct Unary;
struct Literal;
struct Variable;
struct Assign;
struct Input;

struct LetStmt;
struct PrintStmt;
struct IfStmt;
struct WhileStmt;
struct BlockStmt;
struct ExprStmt;

// ── Visitor interfaces ────────────────────────────────────────────────────────

struct ExprVisitor {
    virtual void visitBinaryExpr(Binary& e) = 0;
    virtual void visitUnaryExpr(Unary& e) = 0;
    virtual void visitLiteralExpr(Literal& e) = 0;
    virtual void visitVariableExpr(Variable& e) = 0;
    virtual void visitAssignExpr(Assign& e) = 0;
    virtual void visitInputExpr(Input& e) = 0;
    virtual ~ExprVisitor() = default;
};

struct StmtVisitor {
    virtual void visitLetStmt(LetStmt& s) = 0;
    virtual void visitPrintStmt(PrintStmt& s) = 0;
    virtual void visitIfStmt(IfStmt& s) = 0;
    virtual void visitWhileStmt(WhileStmt& s) = 0;
    virtual void visitBlockStmt(BlockStmt& s) = 0;
    virtual void visitExprStmt(ExprStmt& s) = 0;
    virtual ~StmtVisitor() = default;
};

// ── Base nodes ────────────────────────────────────────────────────────────────

struct Expr {
    int line{0};
    virtual void accept(ExprVisitor& v) = 0;
    virtual ~Expr() = default;
};

struct Stmt {
    int line{0};
    virtual void accept(StmtVisitor& v) = 0;
    virtual ~Stmt() = default;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ── Expression nodes ──────────────────────────────────────────────────────────

struct Binary : Expr {
    ExprPtr left;
    Token   op;     // carries TokenType (for compiler dispatch) and line
    ExprPtr right;
    void accept(ExprVisitor& v) override { v.visitBinaryExpr(*this); }
};

struct Unary : Expr {
    Token   op;
    ExprPtr right;
    void accept(ExprVisitor& v) override { v.visitUnaryExpr(*this); }
};

struct Literal : Expr {
    Value value;
    void accept(ExprVisitor& v) override { v.visitLiteralExpr(*this); }
};

struct Variable : Expr {
    Token name;
    void accept(ExprVisitor& v) override { v.visitVariableExpr(*this); }
};

struct Assign : Expr {
    Token   name;
    ExprPtr value;
    void accept(ExprVisitor& v) override { v.visitAssignExpr(*this); }
};

struct Input : Expr {
    void accept(ExprVisitor& v) override { v.visitInputExpr(*this); }
};

// ── Statement nodes ───────────────────────────────────────────────────────────

struct LetStmt : Stmt {
    Token   name;
    ExprPtr initializer;
    void accept(StmtVisitor& v) override { v.visitLetStmt(*this); }
};

struct PrintStmt : Stmt {
    ExprPtr expression;
    void accept(StmtVisitor& v) override { v.visitPrintStmt(*this); }
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;  // nullptr when there is no else clause
    void accept(StmtVisitor& v) override { v.visitIfStmt(*this); }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;
    void accept(StmtVisitor& v) override { v.visitWhileStmt(*this); }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    void accept(StmtVisitor& v) override { v.visitBlockStmt(*this); }
};

struct ExprStmt : Stmt {
    ExprPtr expression;
    void accept(StmtVisitor& v) override { v.visitExprStmt(*this); }
};
