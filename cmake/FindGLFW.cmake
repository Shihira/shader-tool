if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)

    set(GLFW_LIBRARY ${GLFW_LIBRARIES})
    set(GLFW_INCLUDE_DIR ${GLFW_INCLUDE_DIRS})
endif()

mark_as_advanced(
    GLFW_FOUND
    GLFW_LIBRARY
    GLFW_INCLUDE_DIR
    GLFW_CFLAGS
)

