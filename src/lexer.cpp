#include "cvm/lexer.hpp"
#include <iostream>
#include <unordered_map>

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        start_ = current_;
        Token tok = isAtEnd() ? makeToken(TokenType::END) : scanToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::END) break;
    }
    return tokens;
}

bool Lexer::isAtEnd() const {
    return current_ >= static_cast<int>(source_.size());
}

char Lexer::advance() {
    return source_[current_++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= static_cast<int>(source_.size())) return '\0';
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    ++current_;
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            ++line_;
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // comment: consume to end of line (the '\n' will bump line_ on the next iteration)
            while (!isAtEnd() && peek() != '\n') advance();
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenType type) const {
    return {type, source_.substr(start_, current_ - start_), line_};
}

Token Lexer::errorToken(const std::string& msg) const {
    std::cerr << "[line " << line_ << "] Lexer error: " << msg << '\n';
    return {TokenType::ERROR, msg, line_};
}

Token Lexer::scanNumber() {
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) advance();
    return makeToken(TokenType::NUMBER);
}

TokenType Lexer::keywordType(const std::string& text) const {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"let",   TokenType::LET},
        {"print", TokenType::PRINT},
        {"input", TokenType::INPUT},
        {"if",    TokenType::IF},
        {"else",  TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"true",  TokenType::TRUE},
        {"false", TokenType::FALSE},
    };
    auto it = keywords.find(text);
    return it != keywords.end() ? it->second : TokenType::IDENTIFIER;
}

Token Lexer::scanIdentifierOrKeyword() {
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
        advance();
    std::string text = source_.substr(start_, current_ - start_);
    return makeToken(keywordType(text));
}

Token Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '+': return makeToken(TokenType::PLUS);
        case '-': return makeToken(TokenType::MINUS);
        case '*': return makeToken(TokenType::STAR);
        case '/': return makeToken(TokenType::SLASH);
        case '(': return makeToken(TokenType::LEFT_PAREN);
        case ')': return makeToken(TokenType::RIGHT_PAREN);
        case '{': return makeToken(TokenType::LEFT_BRACE);
        case '}': return makeToken(TokenType::RIGHT_BRACE);
        case ';': return makeToken(TokenType::SEMICOLON);
        case '<': return makeToken(TokenType::LESS);
        case '=': return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        default:
            if (std::isdigit(static_cast<unsigned char>(c))) return scanNumber();
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return scanIdentifierOrKeyword();
            return errorToken(std::string("unexpected character '") + c + "'");
    }
}
