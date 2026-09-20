#ifndef JIT_COMPRESS_H
#define JIT_COMPRESS_H

#include <stddef.h>

#include "jit/strbuf.h"

/**
 * Compresses `len` bytes of `data` and appends the zlib stream to `out`.
 *
 * The stream is produced at compression level 1, which is what git uses for
 * loose objects, so the output matches git's byte for byte.
 *
 * `out` must already be initialized, and `data` must not point into it.
 * A `len` of 0 still produces a valid, non-empty stream.
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 on failure, in which case `out` is unchanged.
 */
int compress_deflate(strbuf_t *out, const void *data, size_t len, strbuf_t *err);

#endif
