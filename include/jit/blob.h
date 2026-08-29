#ifndef JIT_BLOB_H
#define JIT_BLOB_H

#include "jit/strbuf.h"

/**
 * Reads the contents of a file at `path` and stores it in `payload`.
 *
 * `payload` must already be initialized.
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 if the file cannot be read.
 */
int blob_read_path(strbuf_t *payload, const char *path, strbuf_t *err);

#endif