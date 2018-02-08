find_library(ASSIMP_LIBRARY
    NAMES assimp)
find_path(ASSIMP_INCLUDE_DIR assimp/anim.h)

find_package_handle_standard_args(ASSIMP DEFAULT_MSG
    ASSIMP_LIBRARY ASSIMP_INCLUDE_DIR)

mark_as_advanced(
    ASSIMP_LIBRARY
    ASSIMP_INCLUDE_DIR
)

