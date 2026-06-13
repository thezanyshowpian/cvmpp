# CMake generated Testfile for 
# Source directory: /Users/mdadnankhalid/cvmpp
# Build directory: /Users/mdadnankhalid/cvmpp/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(LexerPhase1 "/Users/mdadnankhalid/cvmpp/build/test_lexer")
set_tests_properties(LexerPhase1 PROPERTIES  _BACKTRACE_TRIPLES "/Users/mdadnankhalid/cvmpp/CMakeLists.txt;27;add_test;/Users/mdadnankhalid/cvmpp/CMakeLists.txt;0;")
add_test(ParserPhase2 "/Users/mdadnankhalid/cvmpp/build/test_parser")
set_tests_properties(ParserPhase2 PROPERTIES  _BACKTRACE_TRIPLES "/Users/mdadnankhalid/cvmpp/CMakeLists.txt;35;add_test;/Users/mdadnankhalid/cvmpp/CMakeLists.txt;0;")
add_test(ChunkDisasmPhase3 "/Users/mdadnankhalid/cvmpp/build/test_disassembler")
set_tests_properties(ChunkDisasmPhase3 PROPERTIES  _BACKTRACE_TRIPLES "/Users/mdadnankhalid/cvmpp/CMakeLists.txt;41;add_test;/Users/mdadnankhalid/cvmpp/CMakeLists.txt;0;")
add_test(CompilerPhase4 "/Users/mdadnankhalid/cvmpp/build/test_compiler")
set_tests_properties(CompilerPhase4 PROPERTIES  _BACKTRACE_TRIPLES "/Users/mdadnankhalid/cvmpp/CMakeLists.txt;50;add_test;/Users/mdadnankhalid/cvmpp/CMakeLists.txt;0;")
add_test(VMPhase5 "/Users/mdadnankhalid/cvmpp/build/test_vm")
set_tests_properties(VMPhase5 PROPERTIES  _BACKTRACE_TRIPLES "/Users/mdadnankhalid/cvmpp/CMakeLists.txt;60;add_test;/Users/mdadnankhalid/cvmpp/CMakeLists.txt;0;")
add_test(CLIPhase8 "/Users/mdadnankhalid/cvmpp/tests/test_cli.sh" "/Users/mdadnankhalid/cvmpp/build/cvm" "/Users/mdadnankhalid/cvmpp/scripts")
set_tests_properties(CLIPhase8 PROPERTIES  _BACKTRACE_TRIPLES "/Users/mdadnankhalid/cvmpp/CMakeLists.txt;62;add_test;/Users/mdadnankhalid/cvmpp/CMakeLists.txt;0;")
