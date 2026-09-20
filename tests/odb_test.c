#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "jit/hash.h"
#include "jit/object.h"
#include "jit/odb.h"
#include "jit/strbuf.h"
#include "temp.h"
#include "test.h"

/** Produced by real git 2.50.1 for a blob containing "hello world". */
static const unsigned char HELLO_WORLD_Z[] = {
  0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x34, 0x64, 0xc8, 0x48,
  0xcd, 0xc9, 0xc9, 0x57, 0x28, 0xcf, 0x2f, 0xca, 0x49, 0x01, 0x00, 0x3d,
  0x7b, 0x06, 0x7e
};

static const char *const OID_HELLO_WORLD = "95d09f2b10159347eece71399a7e2e907ea3df4f";
static const char *const OID_EMPTY = "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391";

static char root[PATH_MAX];

static const char *object_path(oid_sha1_t oid) {
  static char path[PATH_MAX];
  oid_path_t components = oid_to_components(oid);

  snprintf(path, sizeof path, "%s/%s/%s", root, components.prefix, components.rest);
  return path;
}

static const char *prefix_path(oid_sha1_t oid) {
  static char path[PATH_MAX];
  oid_path_t components = oid_to_components(oid);

  snprintf(path, sizeof path, "%s/%s", root, components.prefix);
  return path;
}

static int oid_is(oid_sha1_t oid, const char *hex) {
  char got[OID_SHA1_HEXSZ + 1];
  oid_sha1_fmt(got, &oid);
  return strcmp(got, hex) == 0;
}

/** Reads `path` whole. Returns the length, or -1 if it cannot be read. */
static long slurp(const char *path, unsigned char *buf, size_t size) {
  FILE *fp = fopen(path, "rb");
  size_t len;

  if (fp == NULL) return -1;

  len = fread(buf, 1, size, fp);
  fclose(fp);
  return (long)len;
}

static size_t entry_count(const char *path) {
  DIR *dir = opendir(path);
  const struct dirent *entry;
  size_t count = 0;

  if (dir == NULL) return 0;

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
      count++;
  }

  closedir(dir);
  return count;
}

static mode_t mode_of(const char *path) {
  struct stat st;
  if (stat(path, &st) < 0) return 0;
  return st.st_mode & 07777;
}

static ino_t inode_of(const char *path) {
  struct stat st;
  if (stat(path, &st) < 0) return 0;
  return st.st_ino;
}

// storage layout and contents

static void test_writes_at_the_two_character_path(void) {
  oid_sha1_t oid;
  int result, oid_ok, exists;

  temp_root(root, sizeof root, "odb");
  result = odb_write(&oid, root, OBJ_BLOB, "hello world", 11, NULL);
  oid_ok = oid_is(oid, OID_HELLO_WORLD);
  exists = access(object_path(oid), F_OK) == 0;
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(oid_ok);
  ASSERT(exists);
}

/**
 * The stored bytes are the deflated *encoded* object, not the deflated
 * payload. This is the test that catches compressing the wrong buffer.
 */
static void test_file_bytes_match_git(void) {
  oid_sha1_t oid;
  unsigned char got[64];
  long len;
  int result;

  temp_root(root, sizeof root, "odb");
  result = odb_write(&oid, root, OBJ_BLOB, "hello world", 11, NULL);
  len = slurp(object_path(oid), got, sizeof got);
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(len == (long)sizeof HELLO_WORLD_Z);
  ASSERT(memcmp(got, HELLO_WORLD_Z, sizeof HELLO_WORLD_Z) == 0);
}

static void test_writes_an_empty_object(void) {
  oid_sha1_t oid;
  unsigned char got[64];
  unsigned char back[64];
  uLongf back_len = (uLongf)sizeof back;
  long len;
  int result, inflated;

  temp_root(root, sizeof root, "odb");
  result = odb_write(&oid, root, OBJ_BLOB, "", 0, NULL);
  len = slurp(object_path(oid), got, sizeof got);
  inflated = len > 0 && uncompress(back, &back_len, got, (uLong)len) == Z_OK;
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(oid_is(oid, OID_EMPTY));
  ASSERT(inflated);
  ASSERT(back_len == 7);
  ASSERT(memcmp(back, "blob 0\0", 7) == 0);
}

static void test_the_returned_oid_matches_object_hash(void) {
  oid_sha1_t written, expected;
  int result, same;

  temp_root(root, sizeof root, "odb");
  result = odb_write(&written, root, OBJ_BLOB, "some contents\n", 14, NULL);
  object_hash(&expected, OBJ_BLOB, "some contents\n", 14);
  same = oid_sha1_cmp(&written, &expected) == 0;
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(same);
}

static void test_a_payload_with_nul_bytes_roundtrips(void) {
  static const unsigned char payload[] = { 'a', 0x00, 'b' };
  oid_sha1_t oid;
  unsigned char got[64], back[64];
  uLongf back_len = (uLongf)sizeof back;
  long len;
  int result, inflated;

  temp_root(root, sizeof root, "odb");
  result = odb_write(&oid, root, OBJ_BLOB, payload, sizeof payload, NULL);
  len = slurp(object_path(oid), got, sizeof got);
  inflated = len > 0 && uncompress(back, &back_len, got, (uLong)len) == Z_OK;
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(inflated);
  ASSERT(back_len == 10);
  ASSERT(memcmp(back, "blob 3\0a\0b", 10) == 0);
}

static void test_a_large_object_roundtrips(void) {
  const size_t len = 1024 * 1024;
  char *payload = malloc(len);
  unsigned char *stored = malloc(len * 2);
  unsigned char *back = malloc(len * 2);
  strbuf_t expected;
  oid_sha1_t oid;
  long stored_len;
  uLongf back_len = (uLongf)(len * 2);
  int result, inflated, matches;

  if (payload == NULL || stored == NULL || back == NULL) abort();
  for (size_t i = 0; i < len; i++)
    payload[i] = (char)('a' + (i % 26));

  temp_root(root, sizeof root, "odb");
  strbuf_init(&expected, 0);
  object_encode(&expected, OBJ_BLOB, payload, len);

  result = odb_write(&oid, root, OBJ_BLOB, payload, len, NULL);
  stored_len = slurp(object_path(oid), stored, len * 2);
  inflated = stored_len > 0 && uncompress(back, &back_len, stored, (uLong)stored_len) == Z_OK;
  matches = inflated && back_len == expected.len && memcmp(back, expected.buf, expected.len) == 0;

  strbuf_release(&expected);
  free(payload);
  free(stored);
  free(back);
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(matches);
}

// modes and idempotence

static void test_the_object_is_read_only(void) {
  oid_sha1_t oid;
  mode_t mode;
  int result;

  temp_root(root, sizeof root, "odb");
  result = odb_write(&oid, root, OBJ_BLOB, "hello world", 11, NULL);
  mode = mode_of(object_path(oid));
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(mode == 0444);
}

/**
 * Writing an object that is already stored must neither fail against the
 * read-only file nor rewrite it; an unchanged inode proves the short
 * circuit fired rather than a silent replacement.
 */
static void test_writing_twice_does_not_rewrite(void) {
  oid_sha1_t first, second;
  ino_t before, after;
  int one, two;

  temp_root(root, sizeof root, "odb");
  one = odb_write(&first, root, OBJ_BLOB, "hello world", 11, NULL);
  before = inode_of(object_path(first));
  two = odb_write(&second, root, OBJ_BLOB, "hello world", 11, NULL);
  after = inode_of(object_path(second));
  temp_remove(root);

  ASSERT(one == 0);
  ASSERT(two == 0);
  ASSERT(before != 0);
  ASSERT(before == after);
}

static void test_leaves_no_temporary_behind(void) {
  oid_sha1_t oid;
  size_t count;
  int result;

  temp_root(root, sizeof root, "odb");
  result = odb_write(&oid, root, OBJ_BLOB, "hello world", 11, NULL);
  count = entry_count(prefix_path(oid));
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(count == 1);
}

// failure paths

static void test_an_unwritable_prefix_directory_fails_cleanly(void) {
  oid_sha1_t oid;
  strbuf_t err;
  char prefix[PATH_MAX];
  int result;
  size_t err_len, count;

  temp_root(root, sizeof root, "odb");
  strbuf_init(&err, 0);

  /* The first two characters of OID_HELLO_WORLD. */
  snprintf(prefix, sizeof prefix, "%s/95", root);
  if (mkdir(prefix, 0700) < 0) abort();
  if (chmod(prefix, 0500) < 0) abort();

  result = odb_write(&oid, root, OBJ_BLOB, "hello world", 11, &err);
  err_len = err.len;
  count = entry_count(prefix);

  if (chmod(prefix, 0700) < 0) abort();
  strbuf_release(&err);
  temp_remove(root);

  ASSERT(result == -1);
  ASSERT(err_len > 0);
  ASSERT(count == 0);
}

static void test_a_missing_objects_dir_fails(void) {
  oid_sha1_t oid;
  strbuf_t err;
  int result;
  size_t err_len;

  strbuf_init(&err, 0);
  result = odb_write(&oid, "/tmp/jit-odb-does-not-exist", OBJ_BLOB, "hello world", 11, &err);
  err_len = err.len;
  strbuf_release(&err);

  ASSERT(result == -1);
  ASSERT(err_len > 0);
}

static void test_a_null_err_is_accepted(void) {
  oid_sha1_t oid;
  int ok, failed;

  temp_root(root, sizeof root, "odb");
  ok = odb_write(&oid, root, OBJ_BLOB, "hello world", 11, NULL);
  failed = odb_write(&oid, "/tmp/jit-odb-does-not-exist", OBJ_BLOB, "hello world", 11, NULL);
  temp_remove(root);

  ASSERT(ok == 0);
  ASSERT(failed == -1);
}

int main(void) {
  RUN_TEST(test_writes_at_the_two_character_path);
  RUN_TEST(test_file_bytes_match_git);
  RUN_TEST(test_writes_an_empty_object);
  RUN_TEST(test_the_returned_oid_matches_object_hash);
  RUN_TEST(test_a_payload_with_nul_bytes_roundtrips);
  RUN_TEST(test_a_large_object_roundtrips);

  RUN_TEST(test_the_object_is_read_only);
  RUN_TEST(test_writing_twice_does_not_rewrite);
  RUN_TEST(test_leaves_no_temporary_behind);

  if (geteuid() != 0)
    RUN_TEST(test_an_unwritable_prefix_directory_fails_cleanly);
  RUN_TEST(test_a_missing_objects_dir_fails);
  RUN_TEST(test_a_null_err_is_accepted);

  return TEST_SUMMARY();
}
