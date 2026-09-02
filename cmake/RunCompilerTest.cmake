if(NOT DEFINED compiler)
    message(FATAL_ERROR "compiler path not provided")
endif()

if(NOT DEFINED input)
    message(FATAL_ERROR "input file not provided")
endif()

if(NOT DEFINED expect)
    message(FATAL_ERROR "expect mode not provided")
endif()

if(NOT DEFINED stage)
    set(stage "full")
endif()

execute_process(
    COMMAND "${compiler}" "${input}" "--stage" "${stage}"
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(expect STREQUAL "pass")
    if(NOT exit_code EQUAL 0)
        message(FATAL_ERROR
            "expected success for ${input} in stage ${stage}, but compiler exited with ${exit_code}\n"
            "stdout:\n${stdout}\n"
            "stderr:\n${stderr}"
        )
    endif()
elseif(expect STREQUAL "fail")
    if(exit_code EQUAL 0)
        message(FATAL_ERROR
            "expected failure for ${input} in stage ${stage}, but compiler exited successfully\n"
            "stdout:\n${stdout}\n"
            "stderr:\n${stderr}"
        )
    endif()
else()
    message(FATAL_ERROR "unknown expect mode: ${expect}")
endif()
