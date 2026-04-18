# Resolve the project version.
#
# Priority:
#   1. -DNODETALK_VERSION_OVERRIDE=X.Y.Z passed on the cmake command line
#      (used by the Makefile `release` target).
#   2. Top-level `VERSION` file (single line: `X.Y.Z`).
#   3. Latest git tag matching `vX.Y.Z` (via `git describe`).
#   4. Fallback: 0.0.0+dev.

function(nodetalk_detect_version OUT_NUMERIC OUT_FULL)
    set(_version "0.0.0")
    set(_full    "0.0.0+dev")

    if(DEFINED NODETALK_VERSION_OVERRIDE AND NODETALK_VERSION_OVERRIDE)
        set(_version "${NODETALK_VERSION_OVERRIDE}")
        set(_full    "${NODETALK_VERSION_OVERRIDE}")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
        file(READ "${CMAKE_SOURCE_DIR}/VERSION" _file_version)
        string(STRIP "${_file_version}" _file_version)
        if(_file_version MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
            set(_version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
            set(_full    "${_file_version}")
        endif()
    endif()

    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always --match "v*"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE _git_describe
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE _git_rc
        )
        if(_git_rc EQUAL 0 AND _git_describe)
            if(_version STREQUAL "0.0.0")
                string(REGEX MATCH "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)" _m "${_git_describe}")
                if(_m)
                    set(_version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
                endif()
            endif()
            set(_full "${_version} (${_git_describe})")
        endif()
    endif()

    set(${OUT_NUMERIC} "${_version}" PARENT_SCOPE)
    set(${OUT_FULL}    "${_full}"    PARENT_SCOPE)
endfunction()
