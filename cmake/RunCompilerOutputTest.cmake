if(NOT DEFINED compiler OR NOT DEFINED input OR NOT DEFINED expected)
    message(FATAL_ERROR "compiler, input, and expected are required")
endif()

execute_process(
    COMMAND "${compiler}" "${input}" "--emit-ir"
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE stderr
)

if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR
        "IR compilation failed for ${input} with ${exit_code}\n"
        "stderr:\n${stderr}"
    )
endif()

file(READ "${expected}" wanted)
if(NOT actual STREQUAL wanted)
    message(FATAL_ERROR
        "unexpected IR for ${input}\n"
        "expected:\n${wanted}\n"
        "actual:\n${actual}"
    )
endif()
