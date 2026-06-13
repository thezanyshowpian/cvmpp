#pragma once
#include "cvm/chunk.hpp"
#include "cvm/value.hpp"
#include <vector>

class VM {
public:
    int execute(const Chunk& chunk);
private:
    std::vector<Value> stack_;
    void push(Value v);
    Value pop();
    Value peek() const;
};
