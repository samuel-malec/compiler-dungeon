# Runs `compiler input --interpret` and compares what it prints with the first line of the fixture,
#     // expect: <exactly what --interpret must print>
# e.g. `// expect: result: 42`, `// expect: trap: division by zero`, `// expect: out of fuel`.
foreach (var compiler input)
    if (NOT DEFINED ${var})
        message(FATAL_ERROR "${var} is required")
    endif ()
endforeach ()

file(STRINGS "${input}" first_line LIMIT_COUNT 1)
if (NOT first_line MATCHES "^// expect: (.*)$")
    message(FATAL_ERROR "${input}: the first line must be `// expect: <output>`")
endif ()
set(expected "${CMAKE_MATCH_1}")

execute_process(
        COMMAND "${compiler}" "${input}" "--interpret"
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE actual
        ERROR_VARIABLE stderr
)

string(STRIP "${actual}" actual)
if (NOT exit_code EQUAL 0)
    message(FATAL_ERROR "compiler exited with ${exit_code} for ${input}\nstdout:\n${actual}\nstderr:\n${stderr}")
endif ()
if (NOT actual STREQUAL expected)
    message(FATAL_ERROR "${input}\n  expected: ${expected}\n  actual:   ${actual}")
endif ()
