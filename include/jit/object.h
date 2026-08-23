#ifndef JIT_OBJECT_H
#define JIT_OBJECT_H

#include "jit/hash.h"
#include "jit/strbuf.h"

typedef enum {
  OBJ_BLOB
} obj_type_t;

/**
 * Constructs "<object type> <length>\0<data>" and appends to this format to `out`.
 * This is the format used by jit to encode objects (the same format used by git).
 */
void object_encode(strbuf_t *out, obj_type_t type, const void *data, size_t len);

/**
 * Computes the hash of an encoded jit object and stores it in `out`.
 */
void object_hash(oid_sha1_t *out, obj_type_t type, const void *data, size_t len);

#endif