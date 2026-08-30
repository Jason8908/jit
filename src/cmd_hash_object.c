#include <stdio.h>

#include "jit/builtin.h"
#include "jit/usage.h"
#include "jit/strbuf.h"
#include "jit/blob.h"
#include "jit/hash.h"
#include "jit/parse_options.h"

static const char *const hash_object_usage[] = {
  "jit hash-object <file>...",
  "jit hash-object --stdin",
  NULL
};

static int hash_stdin(void);
static int hash_paths(int argc, const char **argv);
static void print_oid(const oid_sha1_t *oid);

int cmd_hash_object(int argc, const char **argv) {
  int from_stdin = 0;

  const option_t options[] = {
    OPT_BOOL(0, "stdin", &from_stdin, "read the object content from stdin"),
    OPT_END()
  };

  argc = parse_options(argc, argv, options, hash_object_usage);

  if (from_stdin) {
    if (argc != 0) {
      error("--stdin takes no file arguments");
      usage_with_options(hash_object_usage, options);
    }

    return hash_stdin();
  }

  if (argc == 0) usage_with_options(hash_object_usage, options);

  return hash_paths(argc, argv);
}

/**
 * Hash the content read from standard input.
 */
static int hash_stdin(void) {
  oid_sha1_t oid;
  strbuf_t err;

  strbuf_init(&err, 0);

  if (blob_hash_stream(&oid, stdin, &err) < 0)
    die("failed to read standard input: %s", err.buf);

  strbuf_release(&err);
  print_oid(&oid);

  return 0;
}

/**
 * Hash each file in `argv`, printing one object id per line.
 *
 * Object ids are printed as they are computed.
 */
static int hash_paths(int argc, const char **argv) {
  for (int i = 0; i < argc; i++) {
    const char *path = argv[i];

    oid_sha1_t oid;
    strbuf_t err;
    strbuf_init(&err, 0);

    int result = blob_hash_path(&oid, path, &err);
    if (result == ERR_FAIL_TO_READ_FILE)
      die("failed to read file '%s': %s", path, err.buf);
    if (result < 0)
      die("failed to create file hash for '%s': %s", path, err.buf);

    strbuf_release(&err);
    print_oid(&oid);
  }

  return 0;
}

static void print_oid(const oid_sha1_t *oid) {
  char hex[OID_SHA1_HEXSZ + 1];

  oid_sha1_fmt(hex, oid);
  printf("%s\n", hex);
}