#pragma once
#include <string>

enum class TokenType {
    // Literals
    NUMBER, IDENTIFIER,
    // Keywords
    LET, PRINT, INPUT, IF, ELSE, WHILE, TRUE, FALSE,
    // Operators
    PLUS, MINUS, STAR, SLASH, EQUAL_EQUAL, LESS, EQUAL,
    // Punctuation
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, SEMICOLON,
    // Special — END avoids conflict with the C <stdio.h> EOF macro
    ERROR, END
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
};
