find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLFW REQUIRED)
find_package(Guile REQUIRED)

file(GLOB_RECURSE ALL_SOURCE src/*.cc src/common/*.cc) 

add_library(${PROJECT_NAME} SHARED ${ALL_SOURCE})
include_directories(${PROJECT_NAME}
    ${OpenGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

file(GLOB_RECURSE ALL_UNIT_TESTS tests/test_*.cc)
file(GLOB_RECURSE ALL_REPL_SOURCE repl/*.cc) 

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GLFW_INCLUDE_DIR}
    ${Guile_INCLUDE_DIR}
    ${OpenGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR})

set(LINK_LIBS
    ${PROJECT_NAME}
    ${Guile_LIBRARY}
    ${OPENGL_gl_LIBRARY}
    ${GLEW_LIBRARY}
    ${GLFW_LIBRARY})

foreach(TEST_FILE ${ALL_UNIT_TESTS})
    get_filename_component(EXEC_NAME ${TEST_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${TEST_FILE})
    include_directories(${EXEC_NAME} ${INCLUDE_DIRS})
    target_link_libraries(${EXEC_NAME} ${LINK_LIBS})
endforeach(TEST_FILE)

find_package(Readline REQUIRED)

add_executable(${PROJECT_NAME}_repl ${ALL_REPL_SOURCE})
include_directories(${PROJECT_NAME}_repl ${INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_repl ${LINK_LIBS} ${Readline_LIBRARY})

