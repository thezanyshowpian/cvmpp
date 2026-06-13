cmake_minimum_required(VERSION 3.16)
project(cvm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

add_executable(cvm src/main.cpp src/lexer.cpp)

enable_testing()
add_executable(test_lexer tests/test_lexer.cpp src/lexer.cpp)
add_test(NAME LexerPhase1 COMMAND test_lexer)
