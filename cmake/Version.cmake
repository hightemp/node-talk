# Resolve the project version from a git tag like `vX.Y.Z`.
# Falls back to 0.0.0+dev if no tag is present.

function(nodetalk_detect_version OUT_NUMERIC OUT_FULL)
    set(_version "0.0.0")
    set(_full    "0.0.0+dev")

    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always --match "v*"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE _git_describe
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _git_rc
        )
        if(_git_rc EQUAL 0 AND _git_describe)
            set(_full "${_git_describe}")
            string(REGEX MATCH "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)" _m "${_git_describe}")
            if(_m)
                set(_version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
            endif()
        endif()
    endif()

    set(${OUT_NUMERIC} "${_version}" PARENT_SCOPE)
    set(${OUT_FULL}    "${_full}"    PARENT_SCOPE)
endfunction()
