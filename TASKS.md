# TASKS.md — CVM++ Build Plan

Build **one phase at a time**. After each phase: build it, pass the acceptance
check, stop and report. The human commits and `/clear`s before the next phase.
Keep the build warning-clean throughout. "Excellent" notes are not optional —
they are what makes this submission stand out.

---

## Phase 0 — Scaffold
- [ ] Directory tree per CLAUDE.md architecture map.
- [ ] `CMakeLists.txt` (single target `cvm`, C++17, `-Wall -Wextra -Wpedantic`).
- [ ] `main.cpp` compiles and prints a banner.
- **Acceptance:** `cmake -B build && cmake --build build` produces a runnable `./build/cvm`.

## Phase 1 — Lexer
- [ ] `TokenType`, `Token`, `Lexer` (string -> vector<Token>).
- [ ] Whitespace, `//` comments, multi-char `==`, line tracking, lexical errors.
- [ ] Unit test for tokenization.
- **Acceptance:** tokenizing `let x = 10 == 5;` yields the correct token stream (tested).

## Phase 2 — Parser + AST
- [ ] AST node types (Binary, Unary, Literal, Variable, Assign, Input;
      Let, Print, If, While, Block, ExprStmt).
- [ ] Recursive-descent parser with correct precedence + panic-mode recovery.
- [ ] AST pretty-printer.
- **Acceptance:** `cvm script.cvm --ast` prints a correct parenthesized tree for
  arithmetic and assignment.

## Phase 3 — Chunk + Disassembler
- [ ] `OpCode` enum, `Chunk` (code, constants, identifiers, per-byte line info).
- [ ] Disassembler prints human-readable instructions.
- **Acceptance:** a hand-built chunk disassembles correctly.
- **Excellent:** disassembly output is aligned and readable (offset, opcode, operand).

## Phase 4 — Compiler (expressions)
- [ ] AST -> bytecode for literals, unary, binary, grouping; constants pool.
- **Acceptance:** `1 + 2 * 3` compiles (verify the bytecode); evaluates to 7 after Phase 5.

## Phase 5 — VM core
- [ ] Value stack, dispatch `switch`, arithmetic/compare/print/pop, `OP_HALT`.
- [ ] Runtime type-error reporting with line numbers.
- **Acceptance:** `scripts/arithmetic.cvm` prints 7, 9, 3, 3, true, true.
- **Excellent:** `1 + true` reports a clean type error, not a crash.

## Phase 6 — Globals & input
- [ ] `let` -> OP_DEFINE_GLOBAL; reads/assigns -> OP_GET/SET_GLOBAL; OP_INPUT.
- [ ] Globals in `std::unordered_map<std::string, Value>` in the VM.
- **Acceptance:** `scripts/variables.cvm` prints 30; undefined-variable errors cleanly.

## Phase 7 — Control flow
- [ ] `if`/`else`, `while`, blocks; jump emission + backpatching + OP_LOOP.
- [ ] Non-bool condition is a runtime error.
- **Acceptance:** `conditionals.cvm`, `loops.cvm`, `fibonacci.cvm` (input 10) all correct.
- **Excellent:** check the disassembly of a loop to confirm jumps/POPs are balanced.

## Phase 8 — CLI polish
- [ ] File runner, REPL, `--ast` / `--bytecode` / `--debug`, exit codes 0/65/70.
- **Acceptance:** every flag and the REPL behave per SPEC §6.

## Phase 9 — Tests, scripts, docs
- [ ] All sample scripts in `scripts/`.
- [ ] Test harness in `tests/` wired into `ctest`, covering each error case in SPEC §8.
- [ ] `README.md`: language overview, opcode table, build/run, example output.
- [ ] `DESIGN.md`: explain the pipeline and the *why* (globals-only, stack-based,
      backpatched jumps, peek-not-pop on conditional jumps).
- **Acceptance:** `ctest` passes; a stranger can build and run in under a minute from the README.

---

## Stretch (only after Phase 9 is fully green)
Pick at most one or two, done cleanly:
- Extra operators: `>`, `<=`, `>=`, `!=`, `!`.
- Logical `and` / `or` with short-circuit jumps.
- Constant folding in the compiler.
- Local variables with stack slots + lexical scoping (this one is large — skip
  unless everything else is polished).

Do NOT attempt functions/call frames — out of scope, high risk.
