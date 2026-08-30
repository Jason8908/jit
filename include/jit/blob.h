#ifndef JIT_BLOB_H
#define JIT_BLOB_H

#include <stdio.h>

#include "jit/hash.h"
#include "jit/strbuf.h"

#define ERR_FAIL_TO_READ_FILE (-100)

/**
 * Reads the contents of a file at `path` and stores it in `payload`.
 *
 * `payload` must already be initialized.
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 if the file cannot be read.
 */
int blob_read_path(strbuf_t *payload, const char *path, strbuf_t *err);

/**
 * Reads the file at `path` and stores its blob object id in `out`.
 *
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 if hashing fails, ERR_FAIL_TO_READ_FILE if the file cannot be read.
 */
int blob_hash_path(oid_sha1_t *out, const char *path, strbuf_t *err);

/**
 * Reads the remaining contents of `fp` and appends them to `payload`.
 *
 * `payload` must already be initialized. `fp` is read to end-of-file but is
 * not closed; the caller keeps ownership of it.
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 if the stream cannot be read.
 */
int blob_read_stream(strbuf_t *payload, FILE *fp, strbuf_t *err);

/**
 * Reads the remaining contents of `fp` and stores its blob object id in `out`.
 *
 * `fp` is read to end-of-file but is not closed.
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, ERR_FAIL_TO_READ_FILE if the stream cannot be read.
 */
int blob_hash_stream(oid_sha1_t *out, FILE *fp, strbuf_t *err);

#endif