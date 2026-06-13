cmake_minimum_required(VERSION 3.16)
project(cvm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

add_executable(cvm
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/disassembler.cpp
    src/vm.cpp
)

enable_testing()

add_executable(test_lexer
    tests/test_lexer.cpp
    src/lexer.cpp
)
add_test(NAME LexerPhase1 COMMAND test_lexer)

add_executable(test_parser
    tests/test_parser.cpp
    src/lexer.cpp
    src/parser.cpp
    src/disassembler.cpp
)
add_test(NAME ParserPhase2 COMMAND test_parser)

add_executable(test_disassembler
    tests/test_disassembler.cpp
    src/disassembler.cpp
)
add_test(NAME ChunkDisasmPhase3 COMMAND test_disassembler)

add_executable(test_compiler
    tests/test_compiler.cpp
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/disassembler.cpp
)
add_test(NAME CompilerPhase4 COMMAND test_compiler)

add_executable(test_vm
    tests/test_vm.cpp
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/disassembler.cpp
    src/vm.cpp
)
add_test(NAME VMPhase5 COMMAND test_vm)
