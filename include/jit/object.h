#ifndef JIT_OBJECT_H
#define JIT_OBJECT_H

#include "jit/hash.h"
#include "jit/strbuf.h"

typedef enum {
  OBJ_BLOB
} obj_type_t;

typedef struct oid_path_t {
  char prefix[2 + 1];
  char rest[OID_SHA1_HEXSZ - 2 + 1];
} oid_path_t;

/**
 * Constructs "<object type> <length>\0<data>" and appends to this format to `out`.
 * This is the format used by jit to encode objects (the same format used by git).
 */
void object_encode(strbuf_t *out, obj_type_t type, const void *data, size_t len);

/**
 * Computes the hash of an encoded jit object and stores it in `out`.
 */
void object_hash(oid_sha1_t *out, obj_type_t type, const void *data, size_t len);

/**
 * Computes the hash of an encoded jit object and stores it in `hash_out`.
 * Also stores the encoded object in `payload_out`.
 *
 * payload_out must be an already initialized string buffer.
 */
void object_hash_and_encode(oid_sha1_t *hash_out, strbuf_t *payload_out, obj_type_t type, const void *data, size_t len);

/**
 * Splits an object ID hash into its components and returns an oid_path_t
 * representing the result.
 */
oid_path_t oid_to_components(oid_sha1_t oid);

#endif