#pragma once
#include <string>
#include <vector>
#include "cvm/token.hpp"

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    int start_{0};
    int current_{0};
    int line_{1};

    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void skipWhitespaceAndComments();
    Token makeToken(TokenType type) const;
    Token errorToken(const std::string& msg) const;
    Token scanToken();
    Token scanNumber();
    Token scanIdentifierOrKeyword();
    TokenType keywordType(const std::string& text) const;
};
