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

