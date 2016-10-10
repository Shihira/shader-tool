cmake_minimum_required(VERSION 3.0)

project(shrtool)

set(CMAKE_BUILD_TYPE DEBUG)

if(UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g")
endif()

add_subdirectory(shrtool)

