cmake_minimum_required(VERSION 3.15)
project(tiger-compiler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

include_directories(${PROJECT_SOURCE_DIR}/include)

# Find Google Test - try system package first, fallback to FetchContent
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(STATUS "System GTest not found, downloading via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
else()
    message(STATUS "Using system GTest")
endif()

set(LEXER_SOURCES
    src/lexer/token.cpp
    src/lexer/lexer.cpp
)

set(AST_SOURCES
    src/ast/ast.cpp
)

set(PARSER_SOURCES
    src/parser/parser.cpp
)

set(SEMANTIC_SOURCES
    src/semantic/types.cpp
    src/semantic/semantic_analyzer.cpp
)

set(ALL_SOURCES
    ${LEXER_SOURCES}
    ${AST_SOURCES}
    ${PARSER_SOURCES}
    ${SEMANTIC_SOURCES}
)

add_library(tiger-lib STATIC ${ALL_SOURCES})

# Main compiler executable
add_executable(tigercc src/main.cpp)
target_link_libraries(tigercc PRIVATE tiger-lib)

# Tests
enable_testing()

# Collect all test source files
set(TEST_SOURCES
    tests/tiger_test.cpp
    tests/parser_test.cpp
    tests/semantic_test.cpp
)

# Main test executable (links all test files together)
add_executable(tiger_tests ${TEST_SOURCES})
if(GTest_FOUND)
    target_link_libraries(tiger_tests PRIVATE tiger-lib gtest gtest_main pthread)
    target_include_directories(tiger_tests PRIVATE ${GTEST_INCLUDE_DIRS})
else()
    target_link_libraries(tiger_tests PRIVATE tiger-lib gtest_main)
endif()

include(GoogleTest)
gtest_discover_tests(tiger_tests)

execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${PROJECT_SOURCE_DIR}/examples
    ${CMAKE_BINARY_DIR}/examples
    ERROR_QUIET)

# Optional: Enable clang-tidy
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    message(STATUS "clang-tidy found: ${CLANG_TIDY}")
    set_target_properties(tiger-lib PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY}")
endif()

# Syntax highlighter tool
add_executable(tiger_highlighter tools/syntax_highlighter.cpp)
target_link_libraries(tiger_highlighter PRIVATE tiger-lib)

# AST dump tool
add_executable(dot_dump tools/dot_dump.cpp)
target_link_libraries(dot_dump PRIVATE tiger-lib)
