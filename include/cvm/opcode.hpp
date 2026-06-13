#pragma once
#include <cstdint>

enum class OpCode : uint8_t {
    OP_CONSTANT,        // 1-byte operand: index into constants pool
    OP_TRUE,
    OP_FALSE,
    OP_NEGATE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQUAL,
    OP_LESS,
    OP_PRINT,
    OP_POP,
    OP_DEFINE_GLOBAL,   // 1-byte operand: index into identifier table
    OP_GET_GLOBAL,      // 1-byte operand: index into identifier table
    OP_SET_GLOBAL,      // 1-byte operand: index into identifier table (peeks stack)
    OP_INPUT,
    OP_JUMP,            // 2-byte big-endian forward offset
    OP_JUMP_IF_FALSE,   // 2-byte big-endian forward offset (peeks stack, does not pop)
    OP_LOOP,            // 2-byte big-endian backward offset
    OP_HALT,
};
