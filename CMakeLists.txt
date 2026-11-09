cmake_minimum_required(VERSION 3.10)
project(compiler-dungeon)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

add_executable(compiler-dungeon
        src/sema/semantic.hpp
        src/sema/types.hpp
        src/analysis/pass.hpp
)
target_sources(compiler-dungeon
        PRIVATE
        src/main.cpp
        src/driver/compiler.cpp
        src/frontend/lexer.cpp
        src/frontend/parser.cpp
        src/common/pretty_printer.cpp
)

if (EMSCRIPTEN)
    add_executable(compiler-dungeon-wasm
            src/wasm/api.cpp
            src/driver/compiler.cpp
            src/frontend/lexer.cpp
            src/frontend/parser.cpp
            src/common/pretty_printer.cpp
    )
    target_compile_options(compiler-dungeon-wasm PRIVATE "-fwasm-exceptions")
    target_link_options(compiler-dungeon-wasm PRIVATE
            "-lembind" "-sMODULARIZE=1" "-sEXPORT_ES6=1"
            "-sENVIRONMENT=web" "-sALLOW_MEMORY_GROWTH=1"
            "-fwasm-exceptions"
    )
endif ()

function(add_compiler_fixture_tests stage)
    file(GLOB_RECURSE stage_good CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/test/${stage}/good/*"
    )
    file(GLOB_RECURSE stage_bad CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/test/${stage}/bad/*"
    )

    foreach (input_file IN LISTS stage_good)
        if (IS_DIRECTORY "${input_file}")
            continue()
        endif ()

        get_filename_component(test_name "${input_file}" NAME_WE)
        add_test(
                NAME ${stage}.good.${test_name}
                COMMAND ${CMAKE_COMMAND}
                -Dcompiler=$<TARGET_FILE:compiler-dungeon>
                -Dinput=${input_file}
                -Dstage=${stage}
                -Dexpect=pass
                -P ${CMAKE_SOURCE_DIR}/cmake/RunCompilerTest.cmake
        )
        set_tests_properties(${stage}.good.${test_name} PROPERTIES LABELS "${stage};good")
    endforeach ()

    foreach (input_file IN LISTS stage_bad)
        if (IS_DIRECTORY "${input_file}")
            continue()
        endif ()

        get_filename_component(test_name "${input_file}" NAME_WE)
        add_test(
                NAME ${stage}.bad.${test_name}
                COMMAND ${CMAKE_COMMAND}
                -Dcompiler=$<TARGET_FILE:compiler-dungeon>
                -Dinput=${input_file}
                -Dstage=${stage}
                -Dexpect=fail
                -P ${CMAKE_SOURCE_DIR}/cmake/RunCompilerTest.cmake
        )
        set_tests_properties(${stage}.bad.${test_name} PROPERTIES LABELS "${stage};bad")
    endforeach ()
endfunction()

add_compiler_fixture_tests(lexer)
add_compiler_fixture_tests(parser)
add_compiler_fixture_tests(semantic)
add_compiler_fixture_tests(typecheck)

# Whole-pipeline tests: compile with every pass enabled and check the printed IR.
# Each test/pipeline/<name>.ks has a sibling <name>.check (see cmake/RunPipelineCheckTest.cmake).
file(GLOB pipeline_inputs CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/test/pipeline/*.ks")
foreach (input_file IN LISTS pipeline_inputs)
    get_filename_component(test_name "${input_file}" NAME_WE)
    add_test(
            NAME pipeline.${test_name}
            COMMAND ${CMAKE_COMMAND}
            -Dcompiler=$<TARGET_FILE:compiler-dungeon>
            -Dinput=${input_file}
            -Dcheck=${CMAKE_SOURCE_DIR}/test/pipeline/${test_name}.check
            -P ${CMAKE_SOURCE_DIR}/cmake/RunPipelineCheckTest.cmake
    )
    set_tests_properties(pipeline.${test_name} PROPERTIES LABELS "pipeline")
endforeach ()

# Exhaustive 8-bit test of the integer semantics shared by constant folding (and, later, an
# interpreter). Not built for the wasm target.
if (NOT EMSCRIPTEN)
    add_executable(int_semantics_test test/unit/int_semantics_test.cpp)
    add_test(NAME unit.int_semantics COMMAND int_semantics_test)
    set_tests_properties(unit.int_semantics PROPERTIES LABELS "unit")
endif ()

# The IR verifier assignment (docs/assignment-ir-verifier.md). `ctest -L verifier` runs it;
# `ctest -LE assignment` skips it. The tests are red until the verifier is written.
if (NOT EMSCRIPTEN)
    add_executable(ir_verifier_test test/unit/ir_verifier_test.cpp)
    add_test(NAME unit.ir_verifier COMMAND ir_verifier_test)
    set_tests_properties(unit.ir_verifier PROPERTIES LABELS "verifier;assignment")
endif ()

# TODO (assignment, Part 6): once `--verify-each` exists, uncomment this to run the verifier after
# every pass over every fixture. Expect red at first: that is the verifier finding real bugs.
# file(GLOB_RECURSE verify_inputs CONFIGURE_DEPENDS
#         "${CMAKE_SOURCE_DIR}/test/cfg/*.ks" "${CMAKE_SOURCE_DIR}/test/ir/*.ks"
#         "${CMAKE_SOURCE_DIR}/test/dce/*.ks" "${CMAKE_SOURCE_DIR}/test/sccp/*.ks"
#         "${CMAKE_SOURCE_DIR}/test/pipeline/*.ks")
# foreach (input_file IN LISTS verify_inputs)
#     file(RELATIVE_PATH rel "${CMAKE_SOURCE_DIR}/test" "${input_file}")
#     string(REGEX REPLACE "[/.]" "_" test_name "${rel}")
#     add_test(NAME verify.${test_name} COMMAND compiler-dungeon ${input_file} --verify-each)
#     set_tests_properties(verify.${test_name} PROPERTIES LABELS "verifier;assignment")
# endforeach ()
