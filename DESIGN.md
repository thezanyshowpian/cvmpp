# DESIGN.md — CVM++ Architecture and Design Decisions

## Overview

CVM++ is a four-stage pipeline. Each stage is a self-contained module with a
clean interface to the next:

```
Source text
    │
    ▼
┌─────────┐     vector<Token>
│  Lexer  │ ──────────────────▶
└─────────┘
                                ┌────────┐     vector<StmtPtr>   (AST)
                                │ Parser │ ──────────────────────▶
                                └────────┘
                                                                   ┌──────────┐    Chunk
                                                                   │ Compiler │ ──────────▶
                                                                   └──────────┘
                                                                                    ┌────┐
                                                                                    │ VM │ ──▶ output
                                                                                    └────┘
```

The split at each boundary is intentional: the lexer knows nothing about grammar;
the parser knows nothing about bytecode; the compiler knows nothing about execution.
This keeps each stage testable in isolation, which is reflected in the five unit-test
binaries wired into CTest.

---

## Stage 1: Lexer (`lexer.hpp / lexer.cpp`)

The lexer is a single-pass character scanner over the source string.

- It maintains a `start` and `current` cursor plus a `line` counter incremented on every `\n`.
- For each token it advances `current` until the token is complete, then builds a
  `Token{type, lexeme, line}`. The `lexeme` is a `std::string_view` slice (or copy
  for keywords), which avoids heap allocation in the hot path.
- `//` comments are consumed in place: the scanner advances to end-of-line and
  produces no token.
- Lexical errors (unknown characters) emit an `ERROR` token with the offending
  character as the lexeme. The parser can then report the error with the line number
  without the lexer needing to know anything about error recovery.

---

## Stage 2: Parser + AST (`parser.hpp / parser.cpp`, `ast.hpp`)

The parser is a hand-written recursive-descent parser with one function per grammar
rule. The grammar is LL(1) everywhere except for assignment, where a one-token
look-ahead at `=` distinguishes `x = …` from a plain identifier expression.

**AST ownership.** Every AST node owns its children via `std::unique_ptr`. This means:
- No manual `delete` anywhere.
- No shared ownership ambiguity — a node has exactly one owner.
- The whole tree is freed when the `vector<StmtPtr>` goes out of scope.

**Panic-mode error recovery.** On a syntax error the parser calls `synchronize()`,
which advances past tokens until it finds a `;` or a statement-starting keyword.
This lets it report multiple errors in one pass rather than stopping at the first.

**AST printer.** `AstPrinter` (in `disassembler.hpp/cpp`) is a visitor over the AST
that produces S-expression strings. It is used by `--ast` and `--debug` and also
drives the parser unit tests, which check the printed form of parsed expressions.

---

## Stage 3: Compiler (`compiler.hpp / compiler.cpp`)

The compiler is a visitor over the AST that emits into a `Chunk`.

### What is a Chunk?

```cpp
struct Chunk {
    std::vector<uint8_t>        code;         // raw bytecode
    std::vector<int>            lines;        // parallel: lines[i] is the source line of code[i]
    std::vector<Value>          constants;    // integer/bool constant pool
    std::vector<std::string>    identifiers;  // variable-name table
};
```

The `lines` array is parallel to `code` (one entry per byte, including operand bytes).
This means the VM can look up the source line for any byte it is currently executing
with `chunk.lines[ip - 1]` — at zero cost, no separate debug-info structure needed.

### Constants pool

Integer literals go into `constants` and are referenced by index. The opcode
`OP_CONSTANT <idx>` takes a 1-byte operand, so up to 256 distinct integer constants
per chunk. Boolean literals skip the pool entirely: `true` → `OP_TRUE`,
`false` → `OP_FALSE`.

### Identifier table

Variable names are stored once in `identifiers`. `identifierIndex(name)` scans the
table and inserts the name only if it is not already present, returning the index.
All global-variable opcodes (`OP_DEFINE_GLOBAL`, `OP_GET_GLOBAL`, `OP_SET_GLOBAL`)
carry a 1-byte index into this table, keeping the bytecode compact. The VM uses
that index to look up the actual string key in an `unordered_map<string, Value>`.

---

## Stage 4: VM (`vm.hpp / vm.cpp`)

The VM is a `switch`-dispatch loop over a flat bytecode array.

```cpp
for (;;) {
    auto instr = static_cast<OpCode>(chunk.code[ip++]);
    int  line  = chunk.lines[ip - 1];
    switch (instr) { … }
}
```

**Value stack.** Operands are pushed and popped from a `std::vector<Value>`. Because
there are no function calls, the stack never grows beyond the depth of the deepest
expression in the source — a few dozen entries at most.

**Globals map.** `std::unordered_map<std::string, Value>` stores all variables. In
the REPL, the map persists across statements (`clearState = false`) so earlier
variable declarations remain in scope.

**Type errors.** Every arithmetic and comparison opcode checks the types of its
operands before performing the operation. On mismatch, `runtimeError(line, msg)`
writes to `stderr` and returns exit code 70. The line number comes directly from
`chunk.lines`, so error messages always pinpoint the source line.

---

## Key design decisions

### 1. Globals-only scope

CVM++ has no lexical scoping. Every variable is a global, stored by name in a
hash map.

**Why:** Local variables in a register or stack-based VM require the compiler to
assign each variable a *slot index* within the current stack frame, and the VM to
maintain frame pointers. That mechanism is correct but adds roughly as much
complexity as the entire rest of the compiler combined. Globals-only makes the
Compiler a straightforward one-pass AST visitor with no environment bookkeeping, and
the VM a single flat dispatch loop with no frame management. The tradeoff is that
variable lookup is a string hash rather than an array index — measurably slower in
a real interpreter, immaterial for a demonstration codebase.

### 2. Stack-based execution

Values are communicated between operations by pushing and popping a stack, not by
naming registers.

**Why:** A register-based VM produces fewer instructions (operands are named, not
repeated pushes), but the compiler needs a register allocator to decide which
virtual register holds each subexpression at any point. That allocator is at least
as complex as the parser. A stack machine encodes the operand order implicitly:
`1 + 2 * 3` compiles to `PUSH 1; PUSH 2; PUSH 3; MUL; ADD`, and every instruction
knows exactly where its inputs are (top of stack) and where its output goes (also
top of stack). This uniformity makes the dispatch loop small and the disassembly
readable without any register-allocation context.

### 3. Backpatched forward jumps

`if` and `while` both require a conditional forward jump, but the compiler emits
the jump opcode *before* it knows how far to jump.

**How:** `emitJump()` writes the opcode and two placeholder bytes `0xFF 0xFF`, then
returns the byte offset of those two bytes. After compiling the branch body,
`patchJump(offset)` computes the actual forward distance
(`chunk.code.size() - offset - 2`) and overwrites the placeholder bytes with the
real value (big-endian, 16-bit). The compiler errors out if the distance exceeds
`UINT16_MAX` (65535 bytes of bytecode, far more than any reasonable CVM++ script).

**Why not a two-pass approach?** A pre-scan to measure branch sizes before emitting
would work but adds a second traversal of the AST. Backpatching achieves the same
result in a single forward pass, which is the standard technique in production
bytecode compilers (Python, Lua, clox).

### 4. Peek-not-pop on `OP_JUMP_IF_FALSE`

`OP_JUMP_IF_FALSE` reads the condition from the top of the stack but does **not**
pop it. The compiler is then responsible for emitting an explicit `OP_POP` on each
branch path.

**Why:** The condition must be discarded once, regardless of which branch is taken.
If `OP_JUMP_IF_FALSE` popped the condition itself, then:
- On the *taken* path (jump happens): one pop is done, which is correct.
- On the *not-taken* path (fall-through): one pop is also done, which is correct.

But the condition needs to be discarded on *both* paths before either branch's body
runs. With a consuming jump, the only way to achieve this is to duplicate the pop
logic in the generated bytecode for both paths, which the compiler must track
manually. That is error-prone and breaks the symmetry of the `if`/`else` bytecode
layout.

With peek-not-pop, the compiler pattern is uniform:

```
  compile condition          ; … → cond
  OP_JUMP_IF_FALSE <else>   ; peek cond; if false, jump
  OP_POP                    ; discard cond — true path
  compile then-branch
  OP_JUMP <end>
<else>:
  OP_COPY                   ; (no-op label)
  OP_POP                    ; discard cond — false path
  compile else-branch (or nothing)
<end>:
```

Each path discards the condition exactly once. The stack is balanced on both sides
of the branch, and the disassembly makes the pop explicit on each path — which
makes the generated code easier to read in `--bytecode` mode.

The same pattern applies to `while`: the condition is peeked by
`OP_JUMP_IF_FALSE` at the top of each iteration and explicitly popped by `OP_POP`
on whichever side of the conditional jump is taken.
