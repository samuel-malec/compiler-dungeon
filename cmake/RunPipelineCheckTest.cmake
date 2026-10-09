# Runs the whole pipeline (--emit-ssa) on `input` and checks the printed IR against `check`.
#
# Each non-empty, non-'#' line of the check file is a literal substring that must appear in the
# output; a line starting with '!' is a substring that must NOT appear. Matching substrings
# rather than whole output keeps these tests independent of value numbering and layout.
foreach (var compiler input check)
    if (NOT DEFINED ${var})
        message(FATAL_ERROR "${var} is required")
    endif ()
endforeach ()

execute_process(
        COMMAND "${compiler}" "${input}" "--emit-ssa"
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE actual
        ERROR_VARIABLE stderr
)

if (NOT exit_code EQUAL 0)
    message(FATAL_ERROR "compiler exited with ${exit_code} for ${input}\nstderr:\n${stderr}")
endif ()

file(READ "${check}" check_text)
string(REPLACE "\n" ";" check_lines "${check_text}")

set(failures "")
foreach (line IN LISTS check_lines)
    string(STRIP "${line}" line)
    if (line STREQUAL "" OR line MATCHES "^#")
        continue()
    endif ()

    if (line MATCHES "^!")
        string(SUBSTRING "${line}" 1 -1 needle)
        string(STRIP "${needle}" needle)
        string(FIND "${actual}" "${needle}" pos)
        if (NOT pos EQUAL -1)
            string(APPEND failures "  unexpected: ${needle}\n")
        endif ()
    else ()
        string(FIND "${actual}" "${line}" pos)
        if (pos EQUAL -1)
            string(APPEND failures "  missing:    ${line}\n")
        endif ()
    endif ()
endforeach ()

if (NOT failures STREQUAL "")
    message(FATAL_ERROR "${input}\n${failures}actual output:\n${actual}")
endif ()
