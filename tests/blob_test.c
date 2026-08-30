#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jit/blob.h"
#include "jit/hash.h"
#include "jit/object.h"
#include "jit/strbuf.h"
#include "test.h"

/**
 * Write `data` to a unique temp file and store its path in `path`.
 *
 * `path` must be a writable template of the form "...XXXXXX" as for mkstemp.
 * Returns 0 on success, -1 on failure.
 */
static int write_temp(char *path, const void *data, size_t len) {
  int fd = mkstemp(path);
  if (fd < 0)
    return -1;
  if (write(fd, data, len) != (ssize_t)len) {
    close(fd);
    unlink(path);
    return -1;
  }
  close(fd);
  return 0;
}

static void test_hash_path_matches_object_hash(void) {
  static const char data[] = "hello world";
  const size_t len = sizeof data - 1;
  char path[] = "/tmp/jit-blob-XXXXXX";
  oid_sha1_t from_path, from_buf;
  strbuf_t err;
  int rc;

  ASSERT(write_temp(path, data, len) == 0);

  strbuf_init(&err, 0);
  rc = blob_hash_path(&from_path, path, &err);
  unlink(path);

  ASSERT(rc == 0);
  strbuf_release(&err);

  object_hash(&from_buf, OBJ_BLOB, data, len);
  ASSERT(oid_sha1_cmp(&from_path, &from_buf) == 0);
}

static void test_hash_path_missing_file(void) {
  oid_sha1_t oid;
  strbuf_t err;

  strbuf_init(&err, 0);
  ASSERT(blob_hash_path(&oid, "no-such-file", &err) == ERR_FAIL_TO_READ_FILE);
  ASSERT(err.len > 0);
  strbuf_release(&err);
}

int main(void) {
  RUN_TEST(test_hash_path_matches_object_hash);
  RUN_TEST(test_hash_path_missing_file);

  return TEST_SUMMARY();
}
