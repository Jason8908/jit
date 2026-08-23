#ifndef JIT_BLOB_H
#define JIT_BLOB_H

#include "jit/strbuf.h"

/**
 * Reads the contents of a file at `path` and stores it in `payload`.
 *
 * `payload` must already be initialized.
 * Returns 0 on success, -1 if file cannot be read.
 */
int blob_read_path(strbuf_t *payload, const char *path);

#endif