#include "cvm/lexer.hpp"
#include <iostream>
#include <vector>

static int failures = 0;

static void check(bool condition, const std::string& desc) {
    if (condition) {
        std::cout << "  PASS  " << desc << '\n';
    } else {
        std::cout << "  FAIL  " << desc << '\n';
        ++failures;
    }
}

static void checkToken(const Token& tok, TokenType expected_type,
                       const std::string& expected_lexeme, int expected_line,
                       const std::string& label) {
    check(tok.type == expected_type,
          label + ": type");
    check(tok.lexeme == expected_lexeme,
          label + ": lexeme='" + expected_lexeme + "' got='" + tok.lexeme + "'");
    check(tok.line == expected_line,
          label + ": line");
}

// Phase 1 acceptance: tokenize "let x = 10 == 5;"
static void testBasicStream() {
    std::cout << "--- testBasicStream ---\n";
    Lexer lex("let x = 10 == 5;");
    std::vector<Token> tokens = lex.tokenize();

    check(tokens.size() == 8, "token count == 8");
    if (tokens.size() < 8) return;

    checkToken(tokens[0], TokenType::LET,        "let", 1, "tokens[0]");
    checkToken(tokens[1], TokenType::IDENTIFIER,  "x",   1, "tokens[1]");
    checkToken(tokens[2], TokenType::EQUAL,       "=",   1, "tokens[2]");
    checkToken(tokens[3], TokenType::NUMBER,      "10",  1, "tokens[3]");
    checkToken(tokens[4], TokenType::EQUAL_EQUAL, "==",  1, "tokens[4]");
    checkToken(tokens[5], TokenType::NUMBER,      "5",   1, "tokens[5]");
    checkToken(tokens[6], TokenType::SEMICOLON,   ";",   1, "tokens[6]");
    checkToken(tokens[7], TokenType::END,         "",    1, "tokens[7]");
}

// Comments are skipped; line numbers track newlines
static void testCommentAndLineTracking() {
    std::cout << "--- testCommentAndLineTracking ---\n";
    Lexer lex("// this is a comment\nlet");
    std::vector<Token> tokens = lex.tokenize();

    check(tokens.size() == 2, "token count == 2 (LET + END)");
    if (tokens.size() < 2) return;

    checkToken(tokens[0], TokenType::LET, "let", 2, "tokens[0] on line 2");
    checkToken(tokens[1], TokenType::END, "",    2, "tokens[1] END");
}

// All single-char and keyword tokens
static void testOperatorsAndPunctuation() {
    std::cout << "--- testOperatorsAndPunctuation ---\n";
    Lexer lex("+ - * / < ( ) { }");
    std::vector<Token> tokens = lex.tokenize();

    check(tokens.size() == 10, "9 operators + END");
    if (tokens.size() < 10) return;

    check(tokens[0].type == TokenType::PLUS,        "PLUS");
    check(tokens[1].type == TokenType::MINUS,       "MINUS");
    check(tokens[2].type == TokenType::STAR,        "STAR");
    check(tokens[3].type == TokenType::SLASH,       "SLASH");
    check(tokens[4].type == TokenType::LESS,        "LESS");
    check(tokens[5].type == TokenType::LEFT_PAREN,  "LEFT_PAREN");
    check(tokens[6].type == TokenType::RIGHT_PAREN, "RIGHT_PAREN");
    check(tokens[7].type == TokenType::LEFT_BRACE,  "LEFT_BRACE");
    check(tokens[8].type == TokenType::RIGHT_BRACE, "RIGHT_BRACE");
}

// All keywords resolve correctly
static void testKeywords() {
    std::cout << "--- testKeywords ---\n";
    Lexer lex("let print input if else while true false");
    std::vector<Token> tokens = lex.tokenize();

    check(tokens.size() == 9, "8 keywords + END");
    if (tokens.size() < 9) return;

    check(tokens[0].type == TokenType::LET,   "LET");
    check(tokens[1].type == TokenType::PRINT,  "PRINT");
    check(tokens[2].type == TokenType::INPUT,  "INPUT");
    check(tokens[3].type == TokenType::IF,     "IF");
    check(tokens[4].type == TokenType::ELSE,   "ELSE");
    check(tokens[5].type == TokenType::WHILE,  "WHILE");
    check(tokens[6].type == TokenType::TRUE,   "TRUE");
    check(tokens[7].type == TokenType::FALSE,  "FALSE");
}

// Unknown character produces an ERROR token; tokenization continues
static void testLexicalError() {
    std::cout << "--- testLexicalError ---\n";
    Lexer lex("@42");
    std::vector<Token> tokens = lex.tokenize();

    check(tokens.size() >= 2, "at least ERROR + NUMBER or similar");
    check(tokens[0].type == TokenType::ERROR, "first token is ERROR");
}

int main() {
    testBasicStream();
    testCommentAndLineTracking();
    testOperatorsAndPunctuation();
    testKeywords();
    testLexicalError();

    std::cout << '\n' << (failures == 0 ? "All tests passed." : "SOME TESTS FAILED.") << '\n';
    return failures == 0 ? 0 : 1;
}
