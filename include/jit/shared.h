#ifndef JIT_SHARED_H
#define JIT_SHARED_H

#include <stdbool.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define GIT_DIR ".git"
#define JIT_DIR ".jit"
#define JIT_OBJECTS_DIR JIT_DIR "/objects"

bool is_jit_repository(const char *path);

/**
 * Create `path` as a directory if it does not exist.
 *
 * Returns true if `path` is a directory afterwards (created or already
 * present). Returns false on error and sets errno. If `path` exists but
 * is not a directory, errno is ENOTDIR.
 */
bool ensure_dir(const char *path);

#endif