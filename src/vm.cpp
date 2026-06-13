#include "cvm/vm.hpp"
#include "cvm/opcode.hpp"
#include <iostream>
#include <string>

static int runtimeError(int line, const std::string& msg) {
    std::cerr << "[line " << line << "] Runtime error: " << msg << '\n';
    return 70;
}

void VM::push(Value v) { stack_.push_back(v); }

Value VM::pop() {
    Value v = stack_.back();
    stack_.pop_back();
    return v;
}

Value VM::peek() const { return stack_.back(); }

int VM::execute(const Chunk& chunk, bool clearState) {
    stack_.clear();
    if (clearState) globals_.clear();
    size_t ip = 0;

    for (;;) {
        auto instr = static_cast<OpCode>(chunk.code[ip++]);
        int line = chunk.lines[ip - 1];

        switch (instr) {

        case OpCode::OP_CONSTANT:
            push(chunk.constants[chunk.code[ip++]]);
            break;

        case OpCode::OP_TRUE:
            push(Value::makeBool(true));
            break;

        case OpCode::OP_FALSE:
            push(Value::makeBool(false));
            break;

        case OpCode::OP_NEGATE: {
            Value a = pop();
            if (a.type != ValueType::INT)
                return runtimeError(line, "operand to unary '-' must be an integer");
            push(Value::makeInt(-a.i));
            break;
        }

        case OpCode::OP_ADD: {
            Value b = pop(), a = pop();
            if (a.type != ValueType::INT || b.type != ValueType::INT)
                return runtimeError(line, "operands to '+' must be integers");
            push(Value::makeInt(a.i + b.i));
            break;
        }

        case OpCode::OP_SUB: {
            Value b = pop(), a = pop();
            if (a.type != ValueType::INT || b.type != ValueType::INT)
                return runtimeError(line, "operands to '-' must be integers");
            push(Value::makeInt(a.i - b.i));
            break;
        }

        case OpCode::OP_MUL: {
            Value b = pop(), a = pop();
            if (a.type != ValueType::INT || b.type != ValueType::INT)
                return runtimeError(line, "operands to '*' must be integers");
            push(Value::makeInt(a.i * b.i));
            break;
        }

        case OpCode::OP_DIV: {
            Value b = pop(), a = pop();
            if (a.type != ValueType::INT || b.type != ValueType::INT)
                return runtimeError(line, "operands to '/' must be integers");
            if (b.i == 0)
                return runtimeError(line, "division by zero");
            push(Value::makeInt(a.i / b.i));
            break;
        }

        case OpCode::OP_EQUAL: {
            Value b = pop(), a = pop();
            if (a.type != b.type)
                return runtimeError(line, "operands to '==' must be the same type");
            bool eq = (a.type == ValueType::INT) ? (a.i == b.i) : (a.b == b.b);
            push(Value::makeBool(eq));
            break;
        }

        case OpCode::OP_LESS: {
            Value b = pop(), a = pop();
            if (a.type != ValueType::INT || b.type != ValueType::INT)
                return runtimeError(line, "operands to '<' must be integers");
            push(Value::makeBool(a.i < b.i));
            break;
        }

        case OpCode::OP_PRINT: {
            Value a = pop();
            if (a.type == ValueType::INT)
                std::cout << a.i << '\n';
            else
                std::cout << (a.b ? "true" : "false") << '\n';
            break;
        }

        case OpCode::OP_POP:
            pop();
            break;

        case OpCode::OP_DEFINE_GLOBAL: {
            uint8_t idx = chunk.code[ip++];
            globals_[chunk.identifiers[idx]] = pop();
            break;
        }

        case OpCode::OP_GET_GLOBAL: {
            uint8_t idx = chunk.code[ip++];
            const std::string& name = chunk.identifiers[idx];
            auto it = globals_.find(name);
            if (it == globals_.end())
                return runtimeError(line, "undefined variable '" + name + "'");
            push(it->second);
            break;
        }

        case OpCode::OP_SET_GLOBAL: {
            uint8_t idx = chunk.code[ip++];
            const std::string& name = chunk.identifiers[idx];
            if (globals_.find(name) == globals_.end())
                return runtimeError(line, "undefined variable '" + name + "'");
            globals_[name] = peek();
            break;
        }

        case OpCode::OP_INPUT: {
            std::string buf;
            if (!std::getline(std::cin, buf))
                return runtimeError(line, "input: failed to read line");
            try {
                size_t pos;
                int64_t val = std::stoll(buf, &pos);
                if (pos != buf.size())
                    return runtimeError(line, "input: not a valid integer");
                push(Value::makeInt(val));
            } catch (...) {
                return runtimeError(line, "input: not a valid integer");
            }
            break;
        }

        case OpCode::OP_JUMP: {
            uint16_t offset = static_cast<uint16_t>(
                (static_cast<uint16_t>(chunk.code[ip]) << 8) | chunk.code[ip + 1]);
            ip += 2 + offset;
            break;
        }

        case OpCode::OP_JUMP_IF_FALSE: {
            uint16_t offset = static_cast<uint16_t>(
                (static_cast<uint16_t>(chunk.code[ip]) << 8) | chunk.code[ip + 1]);
            ip += 2;
            Value cond = peek();
            if (cond.type != ValueType::BOOL)
                return runtimeError(line, "condition must be a boolean");
            if (!cond.b)
                ip += offset;
            break;
        }

        case OpCode::OP_LOOP: {
            uint16_t offset = static_cast<uint16_t>(
                (static_cast<uint16_t>(chunk.code[ip]) << 8) | chunk.code[ip + 1]);
            ip += 2;
            ip -= offset;
            break;
        }

        case OpCode::OP_HALT:
            return 0;

        default:
            return runtimeError(line, "unknown opcode");
        }
    }
}
