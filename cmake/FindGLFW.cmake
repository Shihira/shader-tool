find_library(GLFW_LIBRARY
    NAMES glfw3 glfw)
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)

find_package_handle_standard_args(GLFW DEFAULT_MSG
    GLFW_LIBRARY GLFW_INCLUDE_DIR)

mark_as_advanced(
    GLFW_LIBRARY
    GLFW_INCLUDE_DIR
)

