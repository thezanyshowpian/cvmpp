#pragma once
#include "cvm/chunk.hpp"
#include "cvm/value.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class VM {
public:
    int execute(const Chunk& chunk, bool clearState = true);
private:
    std::vector<Value> stack_;
    std::unordered_map<std::string, Value> globals_;
    void push(Value v);
    Value pop();
    Value peek() const;
};
