#ifndef RR_BUILDID_H
#define RR_BUILDID_H
#include <stddef.h>

/* Compares the loaded GameAssembly.dll against the build this module was
 * compiled for. 1 match, 0 could not check, -1 mismatch (msg filled).
 * On any non-match the result is appended to
 * %LOCALAPPDATA%\RobRunInternal\buildid.log. */
int rr_buildid_check(char* msg, size_t cap);

#endif