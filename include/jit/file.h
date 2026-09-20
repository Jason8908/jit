#ifndef JIT_FILE_H
#define JIT_FILE_H

#include <sys/types.h>

#include "jit/strbuf.h"

/**
 * Write the entire contents of `buf` to `path` with mode `mode`, atomically.
 *
 * The bytes go to a temporary file in the same directory as `path`, which is
 * then renamed over it, so `path` never names a partially written file. A
 * failure leaves no temporary behind. `mode` is set with fchmod and is
 * therefore not filtered through the umask.
 *
 * The directory holding `path` must already exist.
 * `buf` must already be initialized.
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 if the file cannot be written.
 */
int file_write_atomic(const strbuf_t *buf, const char *path, mode_t mode, strbuf_t *err);

#endif
