#ifndef OSDEP_H_INCLUDED
#define OSDEP_H_INCLUDED

// http://stackoverflow.com/a/40230786/2535736

#ifdef _WIN32
#include <direct.h>
// MSDN recommends against using getcwd & chdir names
#define SHRTOOL_GETCWD _getcwd
#define SHRTOOL_CHDIR _chdir
#else
#include "unistd.h"
#define SHRTOOL_GETCWD getcwd
#define SHRTOOL_CHDIR chdir
#endif

#endif // OSDEP_H_INCLUDED
