# SPEC.md — CVM++ Language & Bytecode Specification

## 1. Types
Two value types only: **integer** (`int64_t`) and **boolean**. No strings, no
floats. Number literals are integers.

## 2. Lexical grammar

### Tokens
- Literals: `NUMBER`, `IDENTIFIER`
- Keywords: `let`, `print`, `input`, `if`, `else`, `while`, `true`, `false`
- Operators: `+ - * / == < =`
- Punctuation: `( ) { } ;`
- Special: `EOF`
- Comments: `//` to end of line (lexer skips them).
- Track line numbers; report lexical errors without crashing.

### Syntactic grammar (recursive-descent target, EBNF)
```
program     -> statement* EOF ;
statement   -> letDecl | printStmt | ifStmt | whileStmt | block | exprStmt ;
letDecl     -> "let" IDENTIFIER "=" expression ";" ;
printStmt   -> "print" expression ";" ;
ifStmt      -> "if" "(" expression ")" statement ( "else" statement )? ;
whileStmt   -> "while" "(" expression ")" statement ;
block       -> "{" statement* "}" ;
exprStmt    -> expression ";" ;

expression  -> assignment ;
assignment  -> IDENTIFIER "=" assignment | equality ;
equality    -> comparison ( "==" comparison )* ;
comparison  -> term ( "<" term )* ;
term        -> factor ( ( "+" | "-" ) factor )* ;
factor      -> unary ( ( "*" | "/" ) unary )* ;
unary       -> "-" unary | primary ;
primary     -> NUMBER | "true" | "false" | "input"
             | IDENTIFIER | "(" expression ")" ;
```
Parser uses panic-mode recovery: on error, synchronize at the next `;`.

## 3. Semantics (enforce at compile time or runtime)
- Arithmetic (`+ - * /`, unary `-`) needs both operands **int** -> int.
  `/` is integer division; division by zero is a runtime error.
- `<` needs both operands **int** -> **bool**.
- `==` needs operands of the **same type** -> **bool**.
- `if` / `while` conditions **must** be **bool** — no implicit truthiness;
  a non-bool condition is a runtime error.
- `let x = e;` declares/defines a global; re-declaring overwrites (so `let`
  inside a loop body is legal).
- Assigning (`x = e`) to an **undeclared** name is a runtime error.
  Assignment is an expression and yields the assigned value.
- `input` is an expression: reads one line from stdin, parses it as an integer,
  pushes an int. Malformed input -> runtime error.
- `print e;` prints the value then a newline (`true`/`false` for bools,
  decimal for ints).
- All errors report a line number and a clear message.

## 4. Value representation
```cpp
enum class ValueType { INT, BOOL };
struct Value {
    ValueType type;
    union { int64_t i; bool b; };
    static Value makeInt(int64_t);
    static Value makeBool(bool);
};
```
Small tagged union, no heap allocation (no strings/objects exist).

## 5. Instruction Set (ISA)
One opcode byte + zero or more operand bytes. Globals are referenced by an index
into the chunk's identifier table (`std::vector<std::string>`); numeric literals
by an index into the constants pool (`std::vector<Value>`).

| Opcode | Operands | Stack effect | Meaning |
|---|---|---|---|
| OP_CONSTANT | 1 byte (const idx) | -> push | push constants[idx] |
| OP_TRUE | - | -> push | push bool true |
| OP_FALSE | - | -> push | push bool false |
| OP_NEGATE | - | a -> -a | unary minus (int) |
| OP_ADD / OP_SUB / OP_MUL / OP_DIV | - | a b -> r | int arithmetic |
| OP_EQUAL | - | a b -> bool | equality (same-type) |
| OP_LESS | - | a b -> bool | a < b (int) |
| OP_PRINT | - | a -> | pop & print |
| OP_POP | - | a -> | discard top |
| OP_DEFINE_GLOBAL | 1 byte (name idx) | a -> | pop, store globals[name] |
| OP_GET_GLOBAL | 1 byte (name idx) | -> push | push globals[name] (err if undef) |
| OP_SET_GLOBAL | 1 byte (name idx) | (peek) | globals[name] = peek (err if undef) |
| OP_INPUT | - | -> push | read int from stdin, push |
| OP_JUMP | 2 bytes (u16 fwd) | - | ip += offset |
| OP_JUMP_IF_FALSE | 2 bytes (u16 fwd) | (peek) | if peek==false: ip += offset |
| OP_LOOP | 2 bytes (u16 back) | - | ip -= offset |
| OP_HALT | - | - | stop execution |

Codegen notes:
- `OP_JUMP_IF_FALSE` **peeks** (does not pop). The compiler emits an explicit
  `OP_POP` for the condition on each branch path (standard clox pattern — keep it
  consistent or jumps will subtly corrupt the stack).
- Forward jumps are **backpatched**: emit with placeholder `0xFFFF`, record the
  offset, patch once the target is known.
- 16-bit operands are big-endian (hi, lo). Fail compilation if a jump distance
  exceeds `UINT16_MAX`.

## 6. CLI behavior
```
cvm <script.cvm>             # run a file
cvm                          # REPL (one statement per line)
cvm <script.cvm> --ast       # also print the AST
cvm <script.cvm> --bytecode  # also print disassembled bytecode
cvm <script.cvm> --debug     # print AST + bytecode, then run
```
- Exit codes: 0 success, 65 compile/syntax error, 70 runtime error.
- REPL recovers gracefully from errors and continues looping.

## 7. Sample scripts + expected output (use as acceptance tests)

`scripts/arithmetic.cvm`
```
print 1 + 2 * 3;      // 7
print (1 + 2) * 3;    // 9
print 10 / 3;         // 3
print -5 + 8;         // 3
print 3 < 5;          // true
print 4 == 4;         // true
```

`scripts/variables.cvm`
```
let x = 10;
let y = 20;
x = x + y;
print x;              // 30
```

`scripts/conditionals.cvm`
```
let n = 7;
if (n < 10) { print true; } else { print false; }   // true
```

`scripts/loops.cvm`
```
let i = 0;
while (i < 5) {
  print i;            // 0 1 2 3 4 (each on its own line)
  i = i + 1;
}
```

`scripts/fibonacci.cvm`  (reads count from stdin via input)
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
For input `10`, expected output is: 0 1 1 2 3 5 8 13 21 34 (one per line).

## 8. Error cases that must be handled (not crash)
- `1 + true;`        -> runtime type error w/ line number
- `if (5) {}`        -> runtime type error (condition not bool)
- `print 10 / 0;`    -> runtime error (division by zero)
- `print y;` (y undeclared) -> runtime error (undefined variable)
- `let x = ;`        -> syntax error w/ line number, parser recovers
