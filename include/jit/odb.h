#ifndef JIT_ODB_H
#define JIT_ODB_H

#include <stddef.h>

#include "jit/hash.h"
#include "jit/object.h"
#include "jit/strbuf.h"

/**
 * Encodes, hashes and writes an object into the store rooted at
 * `objects_dir`, storing its object id in `out`.
 *
 * The object is stored compressed at `<objects_dir>/<xx>/<rest>`, where the
 * two-character directory is created if needed. `objects_dir` itself must
 * already exist. Writing an object that is already present succeeds without
 * rewriting it, since a matching name means matching contents.
 *
 * Aborts if `type` is not a known object type, as object_encode does.
 * If `err` is non-NULL it must already be initialized; on failure a
 * reason is appended. Pass NULL to ignore the reason.
 * Returns 0 on success, -1 on failure.
 */
int odb_write(oid_sha1_t *out, const char *objects_dir, obj_type_t type,
              const void *data, size_t len, strbuf_t *err);

#endif
