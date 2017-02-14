cmake_minimum_required(VERSION 3.0)

project(shrtool)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wno-narrowing -O0 -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -Wall -Wno-narrowing -std=c++11 -O0 -g")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-pessimizing-move -Wno-missing-braces")
    endif()

    # https://www.virag.si/2015/07/use-ccache-with-cmake-for-faster-compilation/
    #find_program(CCACHE_FOUND ccache)
    #if(CCACHE_FOUND)
    #    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    #    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    #endif(CCACHE_FOUND)
endif()

add_subdirectory(shrtool)
