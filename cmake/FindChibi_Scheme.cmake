if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(CHIBI_SCHEME REQUIRED chibi-scheme)

    set(Chibi_Scheme_LIBRARY ${CHIBI_SCHEME_LIBRARIES})
    set(Chibi_Scheme_INCLUDE_DIR ${CHIBI_SCHEME_INCLUDE_DIRS})
endif()

mark_as_advanced(
    Chibi_Scheme_FOUND
    Chibi_Scheme_LIBRARY
    Chibi_Scheme_INCLUDE_DIR
    Chibi_Scheme_CFLAGS
)

