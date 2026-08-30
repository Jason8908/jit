#include <string.h>
#include <stdio.h>

#include "jit/builtin.h"
#include "jit/usage.h"
#include "jit/strbuf.h"
#include "jit/blob.h"
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

    oid_sha1_t hash;
    strbuf_t err;
    strbuf_init(&err, 0);

    int hash_result = blob_hash_path(&hash, path, &err);
    if (hash_result == ERR_FAIL_TO_READ_FILE)
      die("failed to read file '%s': %s", path, err.buf);
    if (hash_result < 0)
      die("failed to create file hash for '%s': %s", path, err.buf);
    
    strbuf_release(&err);

    char hex[OID_SHA1_HEXSZ + 1];
    oid_sha1_fmt(hex, &hash);
    printf("%s\n", hex);
  }

  return 0;
}