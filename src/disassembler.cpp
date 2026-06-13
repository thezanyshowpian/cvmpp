#include "cvm/disassembler.hpp"
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
