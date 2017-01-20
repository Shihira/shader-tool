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
        test_prov_shading
        test_render_queue
    )

set(TESTS_REQUIRING_GUILE
        test_shader_parser
        test_prov_shading
        test_scm
        test_render_queue
    )

file(GLOB_RECURSE ALL_UNIT_TESTS tests/test_*.cc)

foreach(TEST_FILE ${ALL_UNIT_TESTS})
    get_filename_component(EXEC_NAME ${TEST_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${TEST_FILE})
    target_link_libraries(${EXEC_NAME} ${PROJECT_NAME})
    include_directories(${EXEC_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src)
endforeach(TEST_FILE)

foreach(EXEC_NAME ${TESTS_REQUIRING_GL_CONTEXT})
    find_package(GLFW REQUIRED)

    include_directories(${EXEC_NAME}
        ${GLEW_INCLUDE_DIR} ${OpenGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR})
    target_link_libraries(${EXEC_NAME}
        ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARY} ${GLFW_LIBRARY})
endforeach(EXEC_NAME)

foreach(EXEC_NAME ${TESTS_REQUIRING_GUILE})
    find_package(Guile REQUIRED)

    include_directories(${EXEC_NAME} ${Guile_INCLUDE_DIR})
    target_link_libraries(${EXEC_NAME} ${Guile_LIBRARY})
endforeach(EXEC_NAME)


