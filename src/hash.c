#if defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/sha.h>
#endif

#include "jit/hash.h"
#include "jit/hex.h"
#include <string.h>

/**
 * Abstract SHA-1 digest implementation for both Apple and OpenSSL.
 */
static void sha1(const void *data, size_t len, unsigned char out[OID_SHA1_RAWSZ]) {
#if defined(__APPLE__)
  CC_SHA1(data, len, out);
#else
  SHA1(data, len, out);
#endif
}

void oid_sha1_hash(oid_sha1_t *oid, const void *data, size_t len) {
  sha1(data, len, oid->hash);
}

void oid_sha1_fmt(char out[OID_SHA1_HEXSZ + 1], const oid_sha1_t *oid) {
  hex_encode(out, oid->hash, OID_SHA1_RAWSZ);
}

int oid_sha1_parse(oid_sha1_t *oid, const char *str) {
  return hex_decode(oid->hash, str, OID_SHA1_HEXSZ);
}

int oid_sha1_cmp(const oid_sha1_t *a, const oid_sha1_t *b) {
  return memcmp(a->hash, b->hash, OID_SHA1_RAWSZ);
}