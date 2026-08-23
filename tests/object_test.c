#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jit/hash.h"
#include "jit/object.h"
#include "jit/strbuf.h"
#include "test.h"

/**
 * Encode `data` into a fresh buffer and report whether the result is exactly
 * the `expected_len` bytes at `expected`.
 *
 * Compared with memcmp rather than strcmp: the encoding carries an embedded
 * NUL separator by design, so it is not a C string.
 *
 * Reports rather than asserts because ASSERT expands to a bare `return`, which
 * inside a helper would hide which case failed.
 */
static int encodes_to(const void *data, size_t len, const void *expected,
                      size_t expected_len) {
  strbuf_t sb;
  int ok;

  strbuf_init(&sb, 0);
  object_encode(&sb, OBJ_BLOB, data, len);

  ok = sb.len == expected_len && memcmp(sb.buf, expected, expected_len) == 0;

  strbuf_release(&sb);
  return ok;
}

static int blob_hashes_to(const void *data, size_t len,
                          const char *expected_hex) {
  oid_sha1_t oid;
  char hex[OID_SHA1_HEXSZ + 1];

  object_hash(&oid, OBJ_BLOB, data, len);
  oid_sha1_fmt(hex, &oid);

  return strcmp(hex, expected_hex) == 0;
}

// object_encode: the format

static void test_encode_blob_header_and_payload(void) {
  ASSERT(encodes_to("abc", 3, "blob 3\0abc", 10));
}

static void test_encode_empty_data(void) {
  ASSERT(encodes_to("", 0, "blob 0\0", 7));
}

static void test_encode_length_is_byte_count_not_strlen(void) {
  // Payload holds an embedded NUL: the header must say 3, and all three
  // bytes must survive the copy.
  ASSERT(encodes_to("a\0b", 3, "blob 3\0a\0b", 10));
}

static void test_encode_separator_is_a_real_nul(void) {
  strbuf_t sb;

  strbuf_init(&sb, 0);
  object_encode(&sb, OBJ_BLOB, "abc", 3);

  ASSERT(sb.len == 10);
  ASSERT(memcmp(sb.buf, "blob 3", 6) == 0);
  ASSERT(sb.buf[6] == '\0');
  ASSERT(memcmp(sb.buf + 7, "abc", 3) == 0);

  strbuf_release(&sb);
}

static void test_encode_large_length_formatting(void) {
  size_t len = 1000;
  char *data = malloc(len);
  strbuf_t sb;

  ASSERT(data != NULL);
  memset(data, 'a', len);

  strbuf_init(&sb, 0);
  object_encode(&sb, OBJ_BLOB, data, len);

  ASSERT(sb.len == 9 + 1 + len);
  ASSERT(memcmp(sb.buf, "blob 1000", 9) == 0);
  ASSERT(sb.buf[9] == '\0');
  ASSERT(memcmp(sb.buf + 10, data, len) == 0);

  strbuf_release(&sb);
  free(data);
}

static void test_encode_appends_to_existing_content(void) {
  strbuf_t sb;

  strbuf_init(&sb, 0);
  strbuf_cat(&sb, "XY", 2);
  object_encode(&sb, OBJ_BLOB, "abc", 3);

  ASSERT(sb.len == 12);
  ASSERT(memcmp(sb.buf, "XYblob 3\0abc", 12) == 0);

  strbuf_release(&sb);
}

static void test_encode_twice_concatenates(void) {
  strbuf_t sb;

  strbuf_init(&sb, 0);
  object_encode(&sb, OBJ_BLOB, "abc", 3);
  object_encode(&sb, OBJ_BLOB, "", 0);

  ASSERT(sb.len == 17);
  ASSERT(memcmp(sb.buf, "blob 3\0abc" "blob 0\0", 17) == 0);

  strbuf_release(&sb);
}

// object_hash: the git compatibility contract

static void test_hash_matches_git_blob_ids(void) {
  static const struct {
    const char *data;
    size_t len;
    const char *oid;
  } vectors[] = {
      {"", 0, "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391"},
      {"abc", 3, "f2ba8f84ab5c1bce84a7b441cb1959cfc7093b7f"},
      {"hello world", 11, "95d09f2b10159347eece71399a7e2e907ea3df4f"},
      {"hello world\n", 12, "3b18e512dba79e4c8300dd08aeb37f8e728b8dad"},
  };

  for (size_t i = 0; i < sizeof vectors / sizeof vectors[0]; i++)
    ASSERT(blob_hashes_to(vectors[i].data, vectors[i].len, vectors[i].oid));
}

static void test_hash_of_binary_data(void) {
  static const unsigned char a_nul_b[] = {'a', 0x00, 'b'};
  static const unsigned char one_nul[] = {0x00};

  ASSERT(blob_hashes_to(a_nul_b, sizeof a_nul_b,
                        "20b5be91886d0b6f26dc98a225c0dac05fe2c86e"));
  ASSERT(blob_hashes_to(one_nul, sizeof one_nul,
                        "f76dd238ade08917e6712764a16a22005a50573d"));
}

static void test_hash_of_large_blob(void) {
  size_t len = 1000;
  char *data = malloc(len);
  int ok;

  ASSERT(data != NULL);
  memset(data, 'a', len);

  ok = blob_hashes_to(data, len, "a50be72b20f0e3f078d252e8e56b11b4bec67509");
  free(data);

  ASSERT(ok);
}

static void test_hash_equals_hash_of_encoded_buffer(void) {
  strbuf_t sb;
  oid_sha1_t from_object, from_buffer;

  strbuf_init(&sb, 0);
  object_encode(&sb, OBJ_BLOB, "some content", 12);
  oid_sha1_hash(&from_buffer, sb.buf, sb.len);
  strbuf_release(&sb);

  object_hash(&from_object, OBJ_BLOB, "some content", 12);

  ASSERT(oid_sha1_cmp(&from_object, &from_buffer) == 0);
}

static void test_hash_is_deterministic(void) {
  oid_sha1_t a, b;

  object_hash(&a, OBJ_BLOB, "repeatable", 10);
  object_hash(&b, OBJ_BLOB, "repeatable", 10);

  ASSERT(oid_sha1_cmp(&a, &b) == 0);
}

static void test_hash_differs_for_different_content(void) {
  oid_sha1_t a, b;

  object_hash(&a, OBJ_BLOB, "abc", 3);
  object_hash(&b, OBJ_BLOB, "abd", 3);

  ASSERT(oid_sha1_cmp(&a, &b) != 0);
}

// failure path

/**
 * Run object_encode with `type` in a forked child and report whether the
 * child died via abort().
 */
static int aborts_on_encode(obj_type_t type) {
  pid_t pid;
  int status;

  // The child inherits stdout's buffer, and abort() may flush it, which
  // would replay every "ok" line printed so far. Drain it before forking.
  fflush(stdout);
  fflush(stderr);

  pid = fork();

  if (pid == -1) return 0;

  if (pid == 0) {
    strbuf_t sb;

    // The parent expects a SIGABRT; anything else (including a clean exit)
    // means the guard did not fire.
    strbuf_init(&sb, 0);
    object_encode(&sb, type, "x", 1);
    strbuf_release(&sb);
    _exit(0);
  }

  if (waitpid(pid, &status, 0) != pid) return 0;

  return WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
}

static void test_encode_aborts_on_unknown_type(void) {
  // Unreachable through the public API today (OBJ_BLOB is the only
  // enumerator), but goes live the moment a type is added without a
  // matching obj_type_str arm.
  ASSERT(aborts_on_encode((obj_type_t)999));
}

int main(void) {
  RUN_TEST(test_encode_blob_header_and_payload);
  RUN_TEST(test_encode_empty_data);
  RUN_TEST(test_encode_length_is_byte_count_not_strlen);
  RUN_TEST(test_encode_separator_is_a_real_nul);
  RUN_TEST(test_encode_large_length_formatting);
  RUN_TEST(test_encode_appends_to_existing_content);
  RUN_TEST(test_encode_twice_concatenates);

  RUN_TEST(test_hash_matches_git_blob_ids);
  RUN_TEST(test_hash_of_binary_data);
  RUN_TEST(test_hash_of_large_blob);
  RUN_TEST(test_hash_equals_hash_of_encoded_buffer);
  RUN_TEST(test_hash_is_deterministic);
  RUN_TEST(test_hash_differs_for_different_content);

  RUN_TEST(test_encode_aborts_on_unknown_type);

  return TEST_SUMMARY();
}
