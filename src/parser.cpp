#include "cvm/parser.hpp"
#include <iostream>
#include <stdexcept>

// Local exception — not part of the public API; callers inspect hadError().
struct ParseError {};

// ── Construction ──────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

bool Parser::hadError() const noexcept { return hadError_; }

// ── Token navigation ──────────────────────────────────────────────────────────

Token& Parser::peek() {
    return tokens_[static_cast<std::size_t>(current_)];
}

Token& Parser::previous() {
    return tokens_[static_cast<std::size_t>(current_ - 1)];
}

bool Parser::isAtEnd() const {
    return tokens_[static_cast<std::size_t>(current_)].type == TokenType::END;
}

bool Parser::check(TokenType t) const {
    if (isAtEnd()) return false;
    return tokens_[static_cast<std::size_t>(current_)].type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

Token Parser::advance() {
    if (!isAtEnd()) ++current_;
    return previous();
}

Token Parser::consume(TokenType t, const char* msg) {
    if (check(t)) return advance();
    error(peek(), msg);
}

// ── Error handling ────────────────────────────────────────────────────────────

void Parser::error(const Token& tok, const char* msg) {
    hadError_ = true;
    std::cerr << "[line " << tok.line << "] Parse error";
    if (tok.type == TokenType::END)
        std::cerr << " at end";
    else
        std::cerr << " at '" << tok.lexeme << "'";
    std::cerr << ": " << msg << '\n';
    throw ParseError{};
}

void Parser::synchronize() {
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        switch (peek().type) {
            case TokenType::LET:
            case TokenType::PRINT:
            case TokenType::IF:
            case TokenType::WHILE:
                return;
            default:
                advance();
        }
    }
}

// ── Entry point ───────────────────────────────────────────────────────────────

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;
    while (!isAtEnd()) {
        StmtPtr s = statement();
        if (s) stmts.push_back(std::move(s));
    }
    return stmts;
}

// ── Statement parsers ─────────────────────────────────────────────────────────

StmtPtr Parser::statement() {
    try {
        if (match(TokenType::LET))        return letDecl();
        if (match(TokenType::PRINT))      return printStmt();
        if (match(TokenType::IF))         return ifStmt();
        if (match(TokenType::WHILE))      return whileStmt();
        if (match(TokenType::LEFT_BRACE)) return block();
        return exprStmt();
    } catch (const ParseError&) {
        synchronize();
        return nullptr;
    }
}

StmtPtr Parser::letDecl() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'.");
    consume(TokenType::EQUAL, "Expected '=' after variable name.");
    ExprPtr init = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after let declaration.");

    auto node = std::make_unique<LetStmt>();
    node->line = name.line;
    node->name = name;
    node->initializer = std::move(init);
    return node;
}

StmtPtr Parser::printStmt() {
    int ln = previous().line;
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after print expression.");

    auto node = std::make_unique<PrintStmt>();
    node->line = ln;
    node->expression = std::move(expr);
    return node;
}

StmtPtr Parser::ifStmt() {
    int ln = previous().line;
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'.");
    ExprPtr cond = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");
    StmtPtr thenB = statement();
    StmtPtr elseB;
    if (match(TokenType::ELSE)) elseB = statement();

    auto node = std::make_unique<IfStmt>();
    node->line = ln;
    node->condition = std::move(cond);
    node->thenBranch = std::move(thenB);
    node->elseBranch = std::move(elseB);
    return node;
}

StmtPtr Parser::whileStmt() {
    int ln = previous().line;
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'.");
    ExprPtr cond = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition.");
    StmtPtr body = statement();

    auto node = std::make_unique<WhileStmt>();
    node->line = ln;
    node->condition = std::move(cond);
    node->body = std::move(body);
    return node;
}

StmtPtr Parser::block() {
    int ln = previous().line;
    auto node = std::make_unique<BlockStmt>();
    node->line = ln;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        StmtPtr s = statement();
        if (s) node->statements.push_back(std::move(s));
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    return node;
}

StmtPtr Parser::exprStmt() {
    int ln = peek().line;
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");

    auto node = std::make_unique<ExprStmt>();
    node->line = ln;
    node->expression = std::move(expr);
    return node;
}

// ── Expression parsers ────────────────────────────────────────────────────────

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    // Two-token lookahead: IDENTIFIER followed by '=' (not '==')?
    // EQUAL and EQUAL_EQUAL are distinct TokenType values, so no ambiguity.
    if (check(TokenType::IDENTIFIER) &&
        static_cast<std::size_t>(current_ + 1) < tokens_.size() &&
        tokens_[static_cast<std::size_t>(current_ + 1)].type == TokenType::EQUAL) {

        Token name = advance();   // consume IDENTIFIER
        advance();                // consume '='
        ExprPtr val = assignment();  // right-recursive → right-associative

        auto node = std::make_unique<Assign>();
        node->line  = name.line;
        node->name  = name;
        node->value = std::move(val);
        return node;
    }
    return equality();
}

ExprPtr Parser::equality() {
    ExprPtr left = comparison();
    while (check(TokenType::EQUAL_EQUAL)) {
        Token op = advance();
        ExprPtr right = comparison();
        auto node = std::make_unique<Binary>();
        node->line  = op.line;
        node->left  = std::move(left);
        node->op    = op;
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

ExprPtr Parser::comparison() {
    ExprPtr left = term();
    while (check(TokenType::LESS)) {
        Token op = advance();
        ExprPtr right = term();
        auto node = std::make_unique<Binary>();
        node->line  = op.line;
        node->left  = std::move(left);
        node->op    = op;
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

ExprPtr Parser::term() {
    ExprPtr left = factor();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = advance();
        ExprPtr right = factor();
        auto node = std::make_unique<Binary>();
        node->line  = op.line;
        node->left  = std::move(left);
        node->op    = op;
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

ExprPtr Parser::factor() {
    ExprPtr left = unary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        Token op = advance();
        ExprPtr right = unary();
        auto node = std::make_unique<Binary>();
        node->line  = op.line;
        node->left  = std::move(left);
        node->op    = op;
        node->right = std::move(right);
        left = std::move(node);
    }
    return left;
}

ExprPtr Parser::unary() {
    if (check(TokenType::MINUS)) {
        Token op = advance();
        ExprPtr right = unary();
        auto node = std::make_unique<Unary>();
        node->line  = op.line;
        node->op    = op;
        node->right = std::move(right);
        return node;
    }
    return primary();
}

ExprPtr Parser::primary() {
    if (match(TokenType::NUMBER)) {
        auto node = std::make_unique<Literal>();
        node->line  = previous().line;
        node->value = Value::makeInt(std::stoll(previous().lexeme));
        return node;
    }
    if (match(TokenType::TRUE)) {
        auto node = std::make_unique<Literal>();
        node->line  = previous().line;
        node->value = Value::makeBool(true);
        return node;
    }
    if (match(TokenType::FALSE)) {
        auto node = std::make_unique<Literal>();
        node->line  = previous().line;
        node->value = Value::makeBool(false);
        return node;
    }
    if (match(TokenType::INPUT)) {
        auto node = std::make_unique<Input>();
        node->line = previous().line;
        return node;
    }
    if (match(TokenType::IDENTIFIER)) {
        auto node = std::make_unique<Variable>();
        node->line = previous().line;
        node->name = previous();
        return node;
    }
    if (match(TokenType::LEFT_PAREN)) {
        ExprPtr inner = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
        return inner;  // grouping is transparent: no wrapper node needed
    }
    error(peek(), "Expected expression.");
}
