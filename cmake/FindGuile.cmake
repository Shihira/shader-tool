if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GUILE REQUIRED guile-2.0)

    set(Guile_LIBRARY ${GUILE_LIBRARIES})
    set(Guile_INCLUDE_DIR ${GUILE_INCLUDE_DIRS})
endif()

mark_as_advanced(
    Guile_FOUND
    Guile_LIBRARY
    Guile_INCLUDE_DIR
    Guile_CFLAGS
)

