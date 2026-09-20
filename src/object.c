#include <stdlib.h>
#include "jit/object.h"
#include "jit/strbuf.h"
#include "jit/hash.h"
#include "jit/hex.h"

static const char *obj_type_str(obj_type_t type) {
  switch (type) {
    case OBJ_BLOB: return "blob";
    default: return NULL;
  }
}

void object_encode(strbuf_t *out, obj_type_t type, const void *data, size_t len) {
  const char *type_str = obj_type_str(type);
  if (type_str == NULL) abort();

  strbuf_catf(out, "%s %zu", type_str, len);
  strbuf_cat(out, "\0", 1);
  strbuf_cat(out, data, len);
}

void object_hash(oid_sha1_t *out, obj_type_t type, const void *data, size_t len) {
  strbuf_t sb;

  strbuf_init(&sb, 0);
  object_encode(&sb, type, data, len);

  oid_sha1_hash(out, sb.buf, sb.len);

  strbuf_release(&sb);
}

void object_hash_and_encode(oid_sha1_t *hash_out, strbuf_t *payload_out, obj_type_t type, const void *data, size_t len) {
  object_encode(payload_out, type, data, len);
  oid_sha1_hash(hash_out, payload_out->buf, payload_out->len);
}

oid_path_t oid_to_components(oid_sha1_t oid) {
  oid_path_t result;

  hex_encode(result.prefix, oid.hash, 1);
  hex_encode(result.rest, oid.hash + 1, OID_SHA1_RAWSZ - 1);

  return result;
}