#ifndef JIT_DIR_H
#define JIT_DIR_H

#include "jit/strbuf.h"

/**
 * Called once per regular file found.
 *
 * `path` is `root` followed by the entry's path beneath it, so it can be
 * opened from the current directory. It is valid only for the duration of
 * the call.
 * Returns 0 to continue the walk, non-zero to stop it.
 */
typedef int (*dir_walk_fn)(const char *path, void *data);

/**
 * Walks `root` recursively, calling `fn` for each regular file.
 *
 * Symlinks are never followed; a symlink is reported as a warning and
 * skipped. Other non-regular entries are skipped silently. The jit
 * repository directory is skipped wherever it is encountered, but `root`
 * itself is not checked; callers that accept user-supplied paths must
 * reject it themselves.
 *
 * An entry that cannot be read is reported as a warning and its subtree is
 * skipped; the rest of the walk still runs.
 *
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 if `root` or any entry beneath it could not be
 * read, or the callback's non-zero return.
 */
int dir_walk(const char *root, dir_walk_fn fn, void *data, strbuf_t *err);

#endif
