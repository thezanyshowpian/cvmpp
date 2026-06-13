# CVM++

A tiny statically-typed scripting language implemented in C++17 to demonstrate
how source text becomes execution: **Lexer → Parser → Compiler → Stack-based VM**.

Two value types. Twenty opcodes. No dependencies.

---

## Quick start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/cvm scripts/arithmetic.cvm   # run a sample script
ctest --test-dir build --output-on-failure  # run all tests
```

Requires CMake ≥ 3.16 and a C++17-capable compiler (GCC 8+, Clang 7+, MSVC 2019+).

---

## Language at a glance

| Feature | Detail |
|---|---|
| **Types** | `int` (64-bit signed integer) and `bool` (`true` / `false`) |
| **Keywords** | `let`, `print`, `input`, `if`, `else`, `while`, `true`, `false` |
| **Operators** | `+ - * /` (int), `<` (int→bool), `==` (same-type→bool), unary `-` |
| **Variables** | Global only; declared with `let x = expr;`; re-declaration overwrites |
| **Assignment** | `x = expr;` — expression that also yields the assigned value |
| **Input** | `input` expression — reads one line from stdin, parses it as an integer |
| **Control flow** | `if (bool-expr) stmt` / `if … else …` / `while (bool-expr) stmt` |
| **Blocks** | `{ stmt* }` — groups statements; no new variable scope |
| **Comments** | `//` to end of line |
| **Semantics** | All type checks at runtime; conditions must be `bool` (no implicit truthiness) |

---

## Sample scripts

### arithmetic.cvm

```
print 1 + 2 * 3;      // 7
print (1 + 2) * 3;    // 9
print 10 / 3;         // 3   (integer division)
print -5 + 8;         // 3
print 3 < 5;          // true
print 4 == 4;         // true
```

```
$ ./build/cvm scripts/arithmetic.cvm
7
9
3
3
true
true
```

### fibonacci.cvm

```
let n = input;
let a = 0;
let b = 1;
let i = 0;
while (i < n) {
  print a;
  let next = a + b;
  a = b;
  b = next;
  i = i + 1;
}
```

```
$ echo 10 | ./build/cvm scripts/fibonacci.cvm
0
1
1
2
3
5
8
13
21
34
```

---

## CLI usage

```
./build/cvm <script.cvm>             # run a file
./build/cvm                          # REPL (one statement per line, Ctrl-D to quit)
./build/cvm <script.cvm> --ast       # print the AST, then run
./build/cvm <script.cvm> --bytecode  # print disassembled bytecode, then run
./build/cvm <script.cvm> --debug     # print AST + bytecode, then run
```

**Exit codes:** `0` success · `65` syntax/compile error · `70` runtime error

---

## `--debug` walkthrough

```
$ echo 'print 1 + 2 * 3;' > /tmp/demo.cvm
$ ./build/cvm /tmp/demo.cvm --debug
```

AST (S-expression format):
```
(print (+ 1 (* 2 3)))
```

Disassembled bytecode:
```
== /tmp/demo.cvm ==
0000    1 OP_CONSTANT            0 '1'
0002    | OP_CONSTANT            1 '2'
0004    | OP_CONSTANT            2 '3'
0006    | OP_MUL
0007    | OP_ADD
0008    | OP_PRINT
0009    | OP_HALT
```

Output:
```
7
```

Offset is decimal, zero-padded. The `|` in the line column means "same line as previous byte". `OP_MUL` before `OP_ADD` reflects precedence: `2 * 3` is compiled first so it sits on top of the stack when `OP_ADD` runs.

---

## Opcode reference

| Opcode | Encoding | Stack effect | Meaning |
|---|---|---|---|
| `OP_CONSTANT` | 1-byte const-pool idx | `→ v` | push `constants[idx]` |
| `OP_TRUE` | — | `→ true` | push bool true |
| `OP_FALSE` | — | `→ false` | push bool false |
| `OP_NEGATE` | — | `a → -a` | unary minus; operand must be int |
| `OP_ADD` | — | `a b → a+b` | integer addition |
| `OP_SUB` | — | `a b → a-b` | integer subtraction |
| `OP_MUL` | — | `a b → a*b` | integer multiplication |
| `OP_DIV` | — | `a b → a/b` | integer division (truncates toward zero) |
| `OP_EQUAL` | — | `a b → bool` | equality; operands must be same type |
| `OP_LESS` | — | `a b → bool` | `a < b`; operands must be int |
| `OP_PRINT` | — | `a →` | pop and print (`true`/`false` or decimal), then newline |
| `OP_POP` | — | `a →` | discard top of stack |
| `OP_DEFINE_GLOBAL` | 1-byte ident-table idx | `a →` | pop; store into `globals[identifiers[idx]]` |
| `OP_GET_GLOBAL` | 1-byte ident-table idx | `→ v` | push `globals[identifiers[idx]]`; runtime error if undeclared |
| `OP_SET_GLOBAL` | 1-byte ident-table idx | (peek) | `globals[identifiers[idx]] = peek()`; runtime error if undeclared |
| `OP_INPUT` | — | `→ v` | read one line from stdin, parse as int, push |
| `OP_JUMP` | 2-byte big-endian u16 | — | unconditional forward jump: `ip += offset` |
| `OP_JUMP_IF_FALSE` | 2-byte big-endian u16 | (peek) | if `peek() == false`: `ip += offset`; does **not** pop (see DESIGN.md) |
| `OP_LOOP` | 2-byte big-endian u16 | — | backward jump: `ip -= offset` |
| `OP_HALT` | — | — | stop execution, return 0 |

---

## Error handling

| Error class | Example trigger | Exit code | Message format |
|---|---|---|---|
| Syntax error | `let x = ;` | 65 | `[line N] Compile error: …` |
| Type mismatch | `1 + true` | 70 | `[line N] Runtime error: …` |
| Division by zero | `print 10 / 0;` | 70 | `[line N] Runtime error: division by zero` |
| Undefined variable | `print y;` | 70 | `[line N] Runtime error: undefined variable 'y'` |
| Non-bool condition | `if (5) {}` | 70 | `[line N] Runtime error: condition must be a boolean` |

---

## Project layout

```
include/cvm/   headers (token, lexer, ast, parser, value, opcode, chunk,
                         compiler, vm, disassembler)
src/           implementation (.cpp for each header)
tests/         unit tests (test_lexer, test_parser, test_disassembler,
                           test_compiler, test_vm) + CLI integration test
scripts/       five sample .cvm programs
CMakeLists.txt single-target build + six CTest targets
```

See [DESIGN.md](DESIGN.md) for a walkthrough of the pipeline and the rationale behind each key design decision.
