#pragma once
#include <cstdint>

enum class ValueType { INT, BOOL };

struct Value {
    ValueType type;
    union { int64_t i; bool b; };

    static Value makeInt(int64_t v) noexcept {
        Value val;
        val.type = ValueType::INT;
        val.i = v;
        return val;
    }
    static Value makeBool(bool v) noexcept {
        Value val;
        val.type = ValueType::BOOL;
        val.b = v;
        return val;
    }
};
