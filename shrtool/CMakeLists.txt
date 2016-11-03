find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

file(GLOB_RECURSE ALL_SOURCE src/*.cc) 
add_library(${PROJECT_NAME} ${ALL_SOURCE})
include_directories(${PROJECT_NAME}
    ${OpenGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

set(TESTS_REQUIRING_GL_CONTEXT
        test_providers
        test_shading
        test_prov_resources
    )

file(GLOB_RECURSE ALL_UNIT_TESTS tests/test_*.cc)

foreach(TEST_FILE ${ALL_UNIT_TESTS})
    find_package(GLFW REQUIRED)

    get_filename_component(EXEC_NAME ${TEST_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${TEST_FILE})
    target_link_libraries(${EXEC_NAME} ${PROJECT_NAME})
    include_directories(${EXEC_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src)

    if(${EXEC_NAME} IN_LIST TESTS_REQUIRING_GL_CONTEXT)
        include_directories(${EXEC_NAME}
            ${GLEW_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR})
        target_link_libraries(${EXEC_NAME}
            ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARY} ${GLFW_LIBRARY})
    endif()
endforeach(TEST_FILE)

