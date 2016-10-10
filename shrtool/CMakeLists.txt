file(GLOB_RECURSE ALL_SOURCE src/*.cc src/*.h)

#add_library(${PROJECT_NAME} ${ALL_SOURCE})

file(GLOB_RECURSE ALL_TESTS tests/*.cc)

foreach(TEST_FILE ${ALL_TESTS})
    get_filename_component(EXEC_NAME ${TEST_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${TEST_FILE})
    #target_link_libraries(${EXEC_NAME} ${PROJECT_NAME})
    include_directories(${EXEC_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src)
endforeach(TEST_FILE)

