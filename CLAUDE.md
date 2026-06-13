# CLAUDE.md — CVM++

Read this file, `SPEC.md`, and `TASKS.md` at the start of every session.

## What this is
CVM++ is a tiny statically-typed scripting language implemented in **C++17**.
Pipeline: **Lexer → Parser (AST) → Compiler (bytecode) → stack-based VM**.
Bytecode is a flat `std::vector<uint8_t>` chunk executed by a switch-dispatch loop.
The design mirrors the *clox* VM from *Crafting Interpreters*, simplified to
**globals-only** (no local-variable slot resolution) to keep scope tight.

The point of the project is to *demonstrate understanding* of how source text
becomes execution, so clarity, good debug output, and clean errors matter as
much as correctness.

## Build / run / test commands
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/cvm scripts/arithmetic.cvm            # run a file
./build/cvm scripts/loops.cvm --debug         # print AST + bytecode + result
./build/cvm                                   # REPL
ctest --test-dir build --output-on-failure    # run tests
```

## Conventions
- C++17, standard library only, **no third-party dependencies**.
- Compile warning-clean with `-Wall -Wextra -Wpedantic`. A warning is a bug.
- Headers in `include/cvm/`, sources in `src/`. Use `#pragma once`.
- No `using namespace std;` in headers.
- AST nodes own children via `std::unique_ptr`.
- Prefer small, readable functions over clever ones. This is a teaching codebase.

## Architecture map
- `token.hpp` — TokenType enum + Token (type, lexeme, line)
- `lexer.{hpp,cpp}` — source string → vector<Token>
- `ast.hpp` — Expr/Stmt node hierarchy
- `parser.{hpp,cpp}` — recursive-descent parser → AST
- `value.hpp` — tagged-union Value (INT | BOOL)
- `opcode.hpp` / `chunk.hpp` — OpCode enum + Chunk (code, constants, identifiers, line info)
- `compiler.{hpp,cpp}` — AST → bytecode (constants pool, jump backpatching)
- `vm.{hpp,cpp}` — value stack + dispatch loop + globals map
- `disassembler.{hpp,cpp}` — debug dumps for AST and bytecode
- `main.cpp` — CLI: file runner, REPL, `--ast` / `--bytecode` / `--debug`

## Working rules (important)
- Build **one phase at a time** per `TASKS.md`. Do not jump ahead.
- After each phase: make it build, pass that phase's acceptance check, then stop
  and report. The human will commit and `/clear` before the next phase.
- When a phase is done, offer a one-paragraph explanation of how it works.
- Ask before any change that alters the architecture in `SPEC.md`.
- Every new feature needs: a sample behavior, an error path, and a test where applicable.

## Definition of done (whole project)
All sample scripts in `scripts/` produce expected output; `--ast`/`--bytecode`/`--debug`
and the REPL work; runtime type errors and syntax errors are reported (not crashes)
with line numbers; exit codes 0 / 65 (compile) / 70 (runtime); README + DESIGN.md written;
tests pass; build is warning-clean.
