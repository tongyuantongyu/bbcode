cmake_minimum_required(VERSION 3.17)
project(bbcode)

set(CMAKE_CXX_STANDARD 20)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-Wall)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions(
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-c++98-c++11-c++14-c++17-compat-pedantic
            -Wno-switch-enum
            -Wno-sign-conversion
            -Wno-shadow-field-in-constructor
            -Wno-padded
            -Wno-documentation-unknown-command
            -Wno-global-constructors
            -Wno-exit-time-destructors
            -Wno-reserved-id-macro
            -Wno-missing-prototypes
    )
endif()

add_library(bbcode_grammar OBJECT grammar.cpp)

add_library(bbcode_lexer lexer.cpp trie.cpp)
target_link_libraries(bbcode_lexer bbcode_grammar)

add_library(bbcode_parser parser.cpp)
target_link_libraries(bbcode_parser bbcode_lexer bbcode_grammar)

option(BBCODE_BUILD_TOOLS "Build tool executables" ON)

if(BBCODE_BUILD_TOOLS)
    add_executable(lexer_tool tools/lexer_tool.cpp)
    target_link_libraries(lexer_tool bbcode_lexer)

    add_executable(parser_tool tools/parser_tool.cpp)
    target_link_libraries(parser_tool bbcode_parser)
endif()

option(BBCODE_BUILD_TESTS "Build tests" OFF)

if(BBCODE_BUILD_TESTS)
    enable_testing()

    add_executable(lexer_test tests/lexer_test.cpp)
    target_link_libraries(lexer_test bbcode_lexer)
    add_test(lexer_test lexer_test)
endif()
