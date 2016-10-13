find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

file(GLOB_RECURSE ALL_SOURCE src/*.cc) 
add_library(${PROJECT_NAME} ${ALL_SOURCE})
include_directories(${PROJECT_NAME}
    ${OpenGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

file(GLOB_RECURSE ALL_UNIT_TESTS tests/test_*.cc)
file(GLOB_RECURSE ALL_GUI_TESTS tests/gui_test_*.cc)

foreach(TEST_FILE ${ALL_UNIT_TESTS})
    get_filename_component(EXEC_NAME ${TEST_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${TEST_FILE})
    target_link_libraries(${EXEC_NAME} ${PROJECT_NAME})
    include_directories(${EXEC_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src)
endforeach(TEST_FILE)

if(ALL_UNIT_TESTS)
    find_package(GLFW REQUIRED)

    foreach(TEST_FILE ${ALL_GUI_TESTS})
        get_filename_component(EXEC_NAME ${TEST_FILE} NAME_WE)
        add_executable(${EXEC_NAME} ${TEST_FILE})
        target_link_libraries(${EXEC_NAME}
            ${PROJECT_NAME} ${OPENGL_gl_LIBRARY}
            ${GLEW_LIBRARY} ${GLFW_LIBRARY})
        include_directories(${EXEC_NAME}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${GLEW_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR}
            ${GLFW_INCLUDE_DIR})
    endforeach(TEST_FILE)
endif()

