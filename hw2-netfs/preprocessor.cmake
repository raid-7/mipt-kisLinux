
function (preprocess_do WHAT INPUT PARAMS OUTVAR DELIM)
    set(_res)
    set(_INPUT_ARR ${INPUT})

    if ("${WHAT}" STREQUAL "prefix")
        foreach(_token ${_INPUT_ARR})
            string(CONCAT _token "${PARAMS}" "${_token}")
            list(APPEND _res "${_token}")
        endforeach()
    endif()

    if ("${WHAT}" STREQUAL "suffix")
        foreach(_token ${_INPUT_ARR})
            string(CONCAT _token "${_token}" "${PARAMS}")
            list(APPEND _res "${_token}")
        endforeach()
    endif()

    if ("${WHAT}" STREQUAL "substitute")
        foreach(_token ${_INPUT_ARR})
            string(REPLACE "#" "${_token}" _token "${PARAMS}")
            list(APPEND _res "${_token}")
        endforeach()
    endif()

    if ("${WHAT}" STREQUAL "iterate")
        list(JOIN _INPUT_ARR "${PARAMS}" _res)
    endif()

    set("${OUTVAR}" ${_res} PARENT_SCOPE)
endfunction()

macro(process_directive _dir _outvar)
    if ("${_dir}" MATCHES "^//!preproc:set:([a-zA-Z0-9_-]+)[ \t]+([^\r\n]+)")
        set(__var "${CMAKE_MATCH_1}")
        string(STRIP "${CMAKE_MATCH_2}" __temp_outvar)
        string(REGEX REPLACE "[ \r\n\t]+" ";" __temp_outvar "${__temp_outvar}")
        set("__preproc_${__var}" "${__temp_outvar}")
        set("${_outvar}")
    endif()

    if ("${_dir}" MATCHES "^//!preproc:get:([a-zA-Z0-9_-]+)[ \t]+([a-zA-Z0-9_-]+)\\(([^\\(\\)\n\r]+)\\)[ \t]*$")
        preprocess_do("${CMAKE_MATCH_2}" "${__preproc_${CMAKE_MATCH_1}}" "${CMAKE_MATCH_3}" __temp_outvar ",")
        set("${_outvar}" ${__temp_outvar})
    endif()

    if ("${_dir}" MATCHES "^//!preproc:get:([a-zA-Z0-9_-]+)[ \t]+([a-zA-Z0-9_-]+)`([^`\n\r]+)`[ \t]*$")
        preprocess_do("${CMAKE_MATCH_2}" "${__preproc_${CMAKE_MATCH_1}}" "${CMAKE_MATCH_3}" __temp_outvar ",")
        set("${_outvar}" ${__temp_outvar})
    endif()

    if ("${_dir}" MATCHES "^//!preproc:transform:([a-zA-Z0-9_-]+):([a-zA-Z0-9_-]+)[ \t]+([a-zA-Z0-9_-]+)\\(([^\\(\\)\n\r]+)\\)[ \t]*$")
        preprocess_do("${CMAKE_MATCH_3}" "${__preproc_${CMAKE_MATCH_1}}" "${CMAKE_MATCH_4}" __temp_outvar ";")
        message(STATUS ":: ${__preproc_${CMAKE_MATCH_1}}  -->  ${__temp_outvar}")
        set("__preproc_${CMAKE_MATCH_2}" ${__temp_outvar})
        set("${_outvar}")
    endif()

    if ("${_dir}" MATCHES "^//!preproc:transform:([a-zA-Z0-9_-]+):([a-zA-Z0-9_-]+)[ \t]+([a-zA-Z0-9_-]+)`([^`\n\r]+)`[ \t]*$")
        preprocess_do("${CMAKE_MATCH_3}" "${__preproc_${CMAKE_MATCH_1}}" "${CMAKE_MATCH_4}" __temp_outvar ";")
        message(STATUS ":: ${__preproc_${CMAKE_MATCH_1}}  -->  ${__temp_outvar}")
        set("__preproc_${CMAKE_MATCH_2}" ${__temp_outvar})
        set("${_outvar}")
    endif()
endmacro()

function(preprocess_file)
    set(optionArgs)
    set(oneValueArgs INPUT OUTPUT)
    set(multiValueArgs)
    cmake_parse_arguments(preprocess_file "${optionArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    file(READ "${preprocess_file_INPUT}" _data)


    while(1)
        string(REGEX MATCH "//!preproc:[^\r\n]+" _dir "${_data}")
        if("${_dir}" MATCHES "^//!preproc")
            process_directive("${_dir}" _res)
            message(STATUS "${_dir}  -->  ${_res}")
            string(REPLACE "${_dir}" "${_res}" _data "${_data}")
        else()
            break()
        endif()
    endwhile()

    file(WRITE ${preprocess_file_OUTPUT} "${_data}")
endfunction()