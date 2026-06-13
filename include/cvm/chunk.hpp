#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "cvm/opcode.hpp"
#include "cvm/value.hpp"

struct Chunk {
    std::vector<uint8_t>     code;
    std::vector<Value>       constants;
    std::vector<std::string> identifiers;
    std::vector<int>         lines;   // parallel to code; one entry per byte

    void writeByte(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    void writeOpCode(OpCode op, int line) {
        writeByte(static_cast<uint8_t>(op), line);
    }

    // Writes two bytes big-endian (hi byte first, lo byte second).
    void writeShort(uint16_t value, int line) {
        writeByte(static_cast<uint8_t>((value >> 8) & 0xFF), line);
        writeByte(static_cast<uint8_t>(value & 0xFF), line);
    }

    int addConstant(Value value) {
        constants.push_back(value);
        return static_cast<int>(constants.size()) - 1;
    }

    int addIdentifier(const std::string& name) {
        identifiers.push_back(name);
        return static_cast<int>(identifiers.size()) - 1;
    }
};
