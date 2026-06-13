#pragma once
#include <vector>
#include "cvm/token.hpp"
#include "cvm/ast.hpp"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    [[nodiscard]] std::vector<StmtPtr> parse();
    [[nodiscard]] bool hadError() const noexcept;

private:
    std::vector<Token> tokens_;
    int current_{0};
    bool hadError_{false};

    // Statement parsers
    StmtPtr statement();
    StmtPtr letDecl();
    StmtPtr printStmt();
    StmtPtr ifStmt();
    StmtPtr whileStmt();
    StmtPtr block();
    StmtPtr exprStmt();

    // Expression parsers (precedence climbing, low → high)
    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr primary();

    // Token navigation
    Token& peek();
    Token& previous();
    bool   isAtEnd() const;
    bool   check(TokenType t) const;
    bool   match(TokenType t);
    Token  consume(TokenType t, const char* msg);
    Token  advance();

    // Error handling
    [[noreturn]] void error(const Token& tok, const char* msg);
    void synchronize();
};
