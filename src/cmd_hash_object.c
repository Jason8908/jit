#include <string.h>
#include <stdio.h>

#include "jit/builtin.h"
#include "jit/usage.h"
#include "jit/strbuf.h"
#include "jit/blob.h"
#include "jit/object.h"
#include "jit/hash.h"

static const char *const hash_object_usage[] = {
  "jit hash-object <path>...",
  NULL
};

int cmd_hash_object(int argc, const char **argv) {
  argc--;
  argv++;

  if (argc == 0) {
    usage(hash_object_usage);
  }

  for (int i = 0; i < argc; i++) {
    const char *path = argv[i];

    strbuf_t payload, err;
    strbuf_init(&payload, 0);
    strbuf_init(&err, 0);
    if (blob_read_path(&payload, path, &err) < 0) {
      die("failed to read file '%s': %s", path, err.buf);
    }
    strbuf_release(&err);

    oid_sha1_t hash;
    object_hash(&hash, OBJ_BLOB, payload.buf, payload.len);
    strbuf_release(&payload);

    char hex[OID_SHA1_HEXSZ + 1];
    oid_sha1_fmt(hex, &hash);
    printf("%s\n", hex);
  }

  return 0;
}