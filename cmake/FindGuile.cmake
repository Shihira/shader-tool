find_library(Guile_LIBRARY guile-2.0)
find_path(Guile_INCLUDE_DIR libguile.h
    PATH_SUFFIXES guile/2.0)

if(NOT ${Guile_LIBRARY})
    find_library(Guile_LIBRARY guile-2.0.dll)
endif()

find_package_handle_standard_args(Guile DEFAULT_MSG
    Guile_LIBRARY Guile_INCLUDE_DIR)

mark_as_advanced(
    Guile_LIBRARY
    Guile_INCLUDE_DIR
    Guile_CFLAGS
)
