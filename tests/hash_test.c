#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "jit/hash.h"
#include "test.h"

#define SENTINEL 0x7e

/**
 * An oid followed by sentinel bytes, so tests can prove oid_sha1_parse writes
 * exactly OID_SHA1_RAWSZ bytes. oid_sha1_t holds a plain unsigned char array,
 * so no padding is inserted before `guard`.
 */
typedef struct {
  oid_sha1_t oid;
  unsigned char guard[16];
} guarded_oid_t;

/**
 * Hash `data` and report whether it formats to `expected_hex`.
 *
 * Returns non-zero on a match. This reports rather than asserts because ASSERT
 * expands to a bare `return`, which inside a helper would hide which vector
 * failed.
 */
static int hashes_to(const void *data, size_t len, const char *expected_hex) {
  oid_sha1_t oid;
  char hex[OID_SHA1_HEXSZ + 1];

  oid_sha1_hash(&oid, data, len);
  oid_sha1_fmt(hex, &oid);

  return strcmp(hex, expected_hex) == 0;
}

static int all_sentinel(const void *buf, size_t n) {
  const unsigned char *p = buf;
  for (size_t i = 0; i < n; i++)
    if (p[i] != SENTINEL) return 0;
  return 1;
}

static void fill_oid(oid_sha1_t *oid, unsigned char value) {
  memset(oid->hash, value, OID_SHA1_RAWSZ);
}

// oid_sha1_hash

static void test_hash_known_vectors(void) {
  static const struct {
    const char *input;
    const char *expected;
  } vectors[] = {
      {"", "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
      {"abc", "a9993e364706816aba3e25717850c26c9cd0d89d"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
       "84983e441c3bd26ebaae4aa1f95129e5e54670f1"},
      {"The quick brown fox jumps over the lazy dog",
       "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"},
  };

  for (size_t i = 0; i < sizeof vectors / sizeof vectors[0]; i++)
    ASSERT(hashes_to(vectors[i].input, strlen(vectors[i].input),
                     vectors[i].expected));
}

static void test_hash_empty_input(void) {
  ASSERT(hashes_to("", 0, "da39a3ee5e6b4b0d3255bfef95601890afd80709"));
}

static void test_hash_binary_data_with_nul(void) {
  static const unsigned char one_nul[] = {0x00};
  static const unsigned char a_nul_b[] = {'a', 0x00, 'b'};

  ASSERT(hashes_to(one_nul, sizeof one_nul,
                   "5ba93c9db0cff93f52b521d7420e43f6eda2784f"));
  ASSERT(hashes_to(a_nul_b, sizeof a_nul_b,
                   "4a3dec2d1f8245280855c42db0ee4239f917fdb8"));
}

static void test_hash_large_input(void) {
  size_t len = 1000000;
  char *data = malloc(len);
  int ok;

  ASSERT(data != NULL);
  memset(data, 'a', len);

  ok = hashes_to(data, len, "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
  free(data);

  ASSERT(ok);
}

static void test_hash_is_deterministic(void) {
  oid_sha1_t a, b;

  oid_sha1_hash(&a, "repeatable", 10);
  oid_sha1_hash(&b, "repeatable", 10);

  ASSERT(oid_sha1_cmp(&a, &b) == 0);
}

static void test_hash_differs_for_different_input(void) {
  oid_sha1_t a, b;

  oid_sha1_hash(&a, "abc", 3);
  oid_sha1_hash(&b, "abd", 3);

  ASSERT(oid_sha1_cmp(&a, &b) != 0);
}

static void test_hash_overwrites_previous_value(void) {
  oid_sha1_t oid;
  char hex[OID_SHA1_HEXSZ + 1];

  fill_oid(&oid, 0xff);
  oid_sha1_hash(&oid, "abc", 3);
  oid_sha1_fmt(hex, &oid);

  ASSERT(strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d") == 0);
}

// oid_sha1_fmt

static void test_fmt_writes_hexsz_chars_and_terminator(void) {
  oid_sha1_t oid;
  char out[64];

  oid_sha1_hash(&oid, "abc", 3);
  memset(out, SENTINEL, sizeof out);
  oid_sha1_fmt(out, &oid);

  ASSERT(strlen(out) == OID_SHA1_HEXSZ);
  ASSERT(out[OID_SHA1_HEXSZ] == '\0');
  ASSERT(all_sentinel(out + OID_SHA1_HEXSZ + 1,
                      sizeof out - OID_SHA1_HEXSZ - 1));
}

static void test_fmt_of_known_oid(void) {
  oid_sha1_t oid;
  char hex[OID_SHA1_HEXSZ + 1];

  for (int i = 0; i < OID_SHA1_RAWSZ; i++)
    oid.hash[i] = (unsigned char)i;

  oid_sha1_fmt(hex, &oid);

  ASSERT(strcmp(hex, "000102030405060708090a0b0c0d0e0f10111213") == 0);
}

// oid_sha1_parse

static void test_parse_accepts_valid_hex(void) {
  static const char *str = "a9993e364706816aba3e25717850c26c9cd0d89d";
  oid_sha1_t oid;

  ASSERT(oid_sha1_parse(&oid, str) == 0);
  ASSERT(oid.hash[0] == 0xa9);
  ASSERT(oid.hash[1] == 0x99);
  ASSERT(oid.hash[OID_SHA1_RAWSZ - 1] == 0x9d);
}

static void test_parse_accepts_uppercase(void) {
  static const char *lower = "a9993e364706816aba3e25717850c26c9cd0d89d";
  char upper[OID_SHA1_HEXSZ + 1];
  oid_sha1_t a, b;

  for (size_t i = 0; i <= OID_SHA1_HEXSZ; i++)
    upper[i] = (char)toupper((unsigned char)lower[i]);

  ASSERT(oid_sha1_parse(&a, lower) == 0);
  ASSERT(oid_sha1_parse(&b, upper) == 0);
  ASSERT(oid_sha1_cmp(&a, &b) == 0);
}

static void test_parse_rejects_invalid_character(void) {
  oid_sha1_t oid;

  ASSERT(oid_sha1_parse(&oid, "z9993e364706816aba3e25717850c26c9cd0d89d") == -1);
  ASSERT(oid_sha1_parse(&oid, "a9993e364706816aba3e25717850c26c9cd0d89z") == -1);
}

static void test_parse_reads_exactly_hexsz_chars(void) {
  // 40 valid hex characters followed by garbage: parse must stop at HEXSZ
  // rather than scanning to the terminator.
  static const char *str = "a9993e364706816aba3e25717850c26c9cd0d89dzzzz";
  oid_sha1_t oid, expected;

  ASSERT(strlen(str) == OID_SHA1_HEXSZ + 4);
  ASSERT(oid_sha1_parse(&oid, str) == 0);

  oid_sha1_hash(&expected, "abc", 3);
  ASSERT(oid_sha1_cmp(&oid, &expected) == 0);
}

static void test_parse_writes_exactly_rawsz_bytes(void) {
  guarded_oid_t g;

  memset(&g, SENTINEL, sizeof g);

  ASSERT(oid_sha1_parse(&g.oid, "a9993e364706816aba3e25717850c26c9cd0d89d") ==
         0);
  ASSERT(all_sentinel(g.guard, sizeof g.guard));
}

// oid_sha1_cmp

static void test_cmp_equal_is_zero(void) {
  oid_sha1_t a, b;

  oid_sha1_hash(&a, "same", 4);
  oid_sha1_hash(&b, "same", 4);

  ASSERT(oid_sha1_cmp(&a, &b) == 0);
  ASSERT(oid_sha1_cmp(&a, &a) == 0);
}

static void test_cmp_orders_by_first_differing_byte(void) {
  oid_sha1_t a, b;

  fill_oid(&a, 0x00);
  fill_oid(&b, 0x00);
  a.hash[0] = 0x01;
  b.hash[0] = 0x02;

  ASSERT(oid_sha1_cmp(&a, &b) < 0);
  ASSERT(oid_sha1_cmp(&b, &a) > 0);
}

static void test_cmp_compares_full_width(void) {
  oid_sha1_t a, b;

  fill_oid(&a, 0x42);
  fill_oid(&b, 0x42);
  a.hash[OID_SHA1_RAWSZ - 1] = 0x01;
  b.hash[OID_SHA1_RAWSZ - 1] = 0x02;

  ASSERT(oid_sha1_cmp(&a, &b) < 0);
  ASSERT(oid_sha1_cmp(&b, &a) > 0);
}

static void test_cmp_is_unsigned(void) {
  oid_sha1_t a, b;

  fill_oid(&a, 0x00);
  fill_oid(&b, 0x00);
  a.hash[0] = 0x7f;
  b.hash[0] = 0x80;

  ASSERT(oid_sha1_cmp(&a, &b) < 0);
  ASSERT(oid_sha1_cmp(&b, &a) > 0);

  a.hash[0] = 0x00;
  b.hash[0] = 0xff;

  ASSERT(oid_sha1_cmp(&a, &b) < 0);
  ASSERT(oid_sha1_cmp(&b, &a) > 0);
}

static void test_cmp_is_antisymmetric(void) {
  static const char *inputs[] = {"alpha", "beta", "gamma", "delta"};

  for (size_t i = 0; i < 4; i++) {
    for (size_t j = i + 1; j < 4; j++) {
      oid_sha1_t a, b;
      int ab, ba;

      oid_sha1_hash(&a, inputs[i], strlen(inputs[i]));
      oid_sha1_hash(&b, inputs[j], strlen(inputs[j]));

      ab = oid_sha1_cmp(&a, &b);
      ba = oid_sha1_cmp(&b, &a);

      ASSERT(ab != 0 && ba != 0);
      ASSERT((ab < 0) == (ba > 0));
    }
  }
}

// composition

static void test_hash_fmt_parse_roundtrip(void) {
  oid_sha1_t original, parsed;
  char hex[OID_SHA1_HEXSZ + 1];

  oid_sha1_hash(&original, "hello world", 11);
  oid_sha1_fmt(hex, &original);

  ASSERT(oid_sha1_parse(&parsed, hex) == 0);
  ASSERT(oid_sha1_cmp(&original, &parsed) == 0);
}

static void test_fmt_parse_roundtrip_boundary_bytes(void) {
  static const unsigned char bytes[OID_SHA1_RAWSZ] = {
      0x00, 0x01, 0x0d, 0x0f, 0x10, 0x7e, 0x7f, 0x80, 0x81, 0x8f,
      0x90, 0xa0, 0xf0, 0xfe, 0xff, 0x20, 0x0a, 0x5c, 0x22, 0x00};
  oid_sha1_t original, parsed;
  char hex[OID_SHA1_HEXSZ + 1];

  memcpy(original.hash, bytes, OID_SHA1_RAWSZ);
  oid_sha1_fmt(hex, &original);

  ASSERT(strlen(hex) == OID_SHA1_HEXSZ);
  ASSERT(oid_sha1_parse(&parsed, hex) == 0);
  ASSERT(oid_sha1_cmp(&original, &parsed) == 0);
}

int main(void) {
  RUN_TEST(test_hash_known_vectors);
  RUN_TEST(test_hash_empty_input);
  RUN_TEST(test_hash_binary_data_with_nul);
  RUN_TEST(test_hash_large_input);
  RUN_TEST(test_hash_is_deterministic);
  RUN_TEST(test_hash_differs_for_different_input);
  RUN_TEST(test_hash_overwrites_previous_value);

  RUN_TEST(test_fmt_writes_hexsz_chars_and_terminator);
  RUN_TEST(test_fmt_of_known_oid);

  RUN_TEST(test_parse_accepts_valid_hex);
  RUN_TEST(test_parse_accepts_uppercase);
  RUN_TEST(test_parse_rejects_invalid_character);
  RUN_TEST(test_parse_reads_exactly_hexsz_chars);
  RUN_TEST(test_parse_writes_exactly_rawsz_bytes);

  RUN_TEST(test_cmp_equal_is_zero);
  RUN_TEST(test_cmp_orders_by_first_differing_byte);
  RUN_TEST(test_cmp_compares_full_width);
  RUN_TEST(test_cmp_is_unsigned);
  RUN_TEST(test_cmp_is_antisymmetric);

  RUN_TEST(test_hash_fmt_parse_roundtrip);
  RUN_TEST(test_fmt_parse_roundtrip_boundary_bytes);

  return TEST_SUMMARY();
}
