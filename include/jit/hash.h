#ifndef JIT_HASH_H
#define JIT_HASH_H

#include <stddef.h>

#define OID_SHA1_RAWSZ 20
#define OID_SHA1_HEXSZ (OID_SHA1_RAWSZ * 2)

/**
 * A content-addressable object id using SHA-1 digest.
 */
typedef struct oid_sha1 {
  unsigned char hash[OID_SHA1_RAWSZ];
} oid_sha1_t;

/**
 * Compute SHA-1 digest of data and store in oid.
 */
void oid_sha1_hash(oid_sha1_t *oid, const void *data, size_t len);

/**
 * Write hex string representation of oid to `out`.
 * The output string is null-terminated, so `out` must be at least OID_SHA1_HEXSZ + 1
 * bytes long.
 */
void oid_sha1_fmt(char out[OID_SHA1_HEXSZ + 1], const oid_sha1_t *oid);

/**
 * Parse hex string representation of oid into oid.
 * Returns 0 on success, -1 on failure.
 * On failure, oid is unspecified.
 *
 * `str` must be exactly OID_SHA1_HEXSZ characters long; otherwise,
 * the behavior is undefined.
 */
int oid_sha1_parse(oid_sha1_t *oid, const char *str);

/**
 * Compare two oid_sha1_t objects.
 * Returns 0 if the objects are equal, positive if a > b, negative if a < b.
 */
int oid_sha1_cmp(const oid_sha1_t *a, const oid_sha1_t *b);

#endif