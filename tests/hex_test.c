#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "jit/hex.h"
#include "test.h"

#define SENTINEL 0x7e

/**
 * Neither hex function takes the length of `out`, so tests pre-fill the
 * output buffer with a sentinel byte and check that the bytes past the
 * documented end are still untouched.
 */
static void fill_sentinel(void *buf, size_t n) { memset(buf, SENTINEL, n); }

static int all_sentinel(const void *buf, size_t n) {
  const unsigned char *p = buf;
  for (size_t i = 0; i < n; i++)
    if (p[i] != SENTINEL) return 0;
  return 1;
}

static int is_hex_digit(int c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

// hex_encode

static void test_encode_all_byte_values(void) {
  for (int i = 0; i < 256; i++) {
    unsigned char byte = (unsigned char)i;
    char expected[3];
    char out[3];

    snprintf(expected, sizeof expected, "%02x", i);
    hex_encode(out, &byte, 1);

    ASSERT(memcmp(out, expected, 3) == 0);
  }
}

static void test_encode_nibble_order_is_high_first(void) {
  unsigned char lo = 0x1f;
  unsigned char hi = 0xf1;
  char out[3];

  hex_encode(out, &lo, 1);
  ASSERT(strcmp(out, "1f") == 0);

  hex_encode(out, &hi, 1);
  ASSERT(strcmp(out, "f1") == 0);
}

static void test_encode_uses_lowercase(void) {
  unsigned char data[] = {0xab, 0xcd, 0xef};
  char out[7];

  hex_encode(out, data, sizeof data);

  ASSERT(strcmp(out, "abcdef") == 0);
}

static void test_encode_multi_byte_sequence(void) {
  unsigned char data[] = {0x00, 0x01, 0x7f, 0x80, 0xff};
  char out[11];

  hex_encode(out, data, sizeof data);

  ASSERT(strcmp(out, "00017f80ff") == 0);
}

static void test_encode_nul_terminates(void) {
  unsigned char data[] = {0x00, 0x41, 0x00};
  char out[7];

  hex_encode(out, data, sizeof data);

  ASSERT(out[6] == '\0');
  ASSERT(strlen(out) == 6);
  ASSERT(strcmp(out, "004100") == 0);
}

static void test_encode_zero_length_writes_only_terminator(void) {
  unsigned char data[] = {0xff};
  char out[8];

  fill_sentinel(out, sizeof out);
  hex_encode(out, data, 0);

  ASSERT(out[0] == '\0');
  ASSERT(all_sentinel(out + 1, sizeof out - 1));
}

static void test_encode_writes_exactly_2n_plus_1(void) {
  unsigned char data[] = {0x01, 0x02, 0x03};
  char out[16];

  fill_sentinel(out, sizeof out);
  hex_encode(out, data, sizeof data);

  ASSERT(strcmp(out, "010203") == 0);
  ASSERT(all_sentinel(out + 7, sizeof out - 7));
}

// hex_decode

static void test_decode_all_byte_pairs(void) {
  for (int i = 0; i < 256; i++) {
    char hex[3];
    unsigned char out = 0;

    snprintf(hex, sizeof hex, "%02x", i);

    ASSERT(hex_decode(&out, hex, 2) == 0);
    ASSERT(out == (unsigned char)i);
  }
}

static void test_decode_accepts_uppercase(void) {
  unsigned char out[3];

  ASSERT(hex_decode(out, "ABCDEF", 6) == 0);
  ASSERT(out[0] == 0xab && out[1] == 0xcd && out[2] == 0xef);
}

static void test_decode_accepts_mixed_case(void) {
  unsigned char out[3];

  ASSERT(hex_decode(out, "aBcDeF", 6) == 0);
  ASSERT(out[0] == 0xab && out[1] == 0xcd && out[2] == 0xef);
}

static void test_decode_zero_length_succeeds(void) {
  unsigned char out[4];

  fill_sentinel(out, sizeof out);

  ASSERT(hex_decode(out, "", 0) == 0);
  ASSERT(all_sentinel(out, sizeof out));
}

static void test_decode_odd_length_fails(void) {
  // Deliberately not NUL-terminated, and every character is valid hex, so
  // only the length check can reject these. A string literal would fail for
  // the wrong reason: the parser would read the terminator and see a
  // non-hex character.
  static const char hex[] = {'a', 'b', 'c', 'd', 'e', 'f'};
  unsigned char out[4];

  ASSERT(hex_decode(out, hex, 1) == -1);
  ASSERT(hex_decode(out, hex, 3) == -1);
  ASSERT(hex_decode(out, hex, 5) == -1);
}

static void test_decode_rejects_invalid_characters(void) {
  static const char invalid[] = {'/',  ':', '@',  'G', '`',
                                 'g',  ' ', '\0', '-', (char)0x80};

  for (size_t i = 0; i < sizeof invalid; i++) {
    unsigned char out[2];
    char high[2] = {invalid[i], '0'};
    char low[2] = {'0', invalid[i]};

    ASSERT(hex_decode(out, high, 2) == -1);
    ASSERT(hex_decode(out, low, 2) == -1);
  }
}

static void test_decode_rejects_invalid_char_in_last_pair(void) {
  unsigned char out[3];

  ASSERT(hex_decode(out, "41ff4z", 6) == -1);
}

static void test_decode_char_acceptance_is_exhaustive(void) {
  for (int c = 0; c < 256; c++) {
    unsigned char out[1];
    char pair[2] = {(char)c, '0'};
    int rc = hex_decode(out, pair, 2);

    ASSERT(rc == (is_hex_digit(c) ? 0 : -1));
  }
}

static void test_decode_ignores_input_past_hex_len(void) {
  unsigned char out[2];

  ASSERT(hex_decode(out, "41ffzz", 4) == 0);
  ASSERT(out[0] == 0x41 && out[1] == 0xff);
}

static void test_decode_writes_exactly_half_length(void) {
  unsigned char out[16];

  fill_sentinel(out, sizeof out);

  ASSERT(hex_decode(out, "41424344", 8) == 0);
  ASSERT(memcmp(out, "ABCD", 4) == 0);
  ASSERT(all_sentinel(out + 4, sizeof out - 4));
}

// round trip

static void test_roundtrip_all_bytes(void) {
  unsigned char data[256];
  unsigned char decoded[256];
  char hex[513];

  for (int i = 0; i < 256; i++) data[i] = (unsigned char)i;

  hex_encode(hex, data, sizeof data);

  ASSERT(strlen(hex) == 512);
  ASSERT(hex_decode(decoded, hex, 512) == 0);
  ASSERT(memcmp(decoded, data, sizeof data) == 0);
}

static void test_roundtrip_uppercased_hex(void) {
  unsigned char data[256];
  unsigned char decoded[256];
  char hex[513];

  for (int i = 0; i < 256; i++) data[i] = (unsigned char)i;

  hex_encode(hex, data, sizeof data);
  for (size_t i = 0; i < 512; i++)
    hex[i] = (char)toupper((unsigned char)hex[i]);

  ASSERT(hex_decode(decoded, hex, 512) == 0);
  ASSERT(memcmp(decoded, data, sizeof data) == 0);
}

int main(void) {
  RUN_TEST(test_encode_all_byte_values);
  RUN_TEST(test_encode_nibble_order_is_high_first);
  RUN_TEST(test_encode_uses_lowercase);
  RUN_TEST(test_encode_multi_byte_sequence);
  RUN_TEST(test_encode_nul_terminates);
  RUN_TEST(test_encode_zero_length_writes_only_terminator);
  RUN_TEST(test_encode_writes_exactly_2n_plus_1);

  RUN_TEST(test_decode_all_byte_pairs);
  RUN_TEST(test_decode_accepts_uppercase);
  RUN_TEST(test_decode_accepts_mixed_case);
  RUN_TEST(test_decode_zero_length_succeeds);
  RUN_TEST(test_decode_odd_length_fails);
  RUN_TEST(test_decode_rejects_invalid_characters);
  RUN_TEST(test_decode_rejects_invalid_char_in_last_pair);
  RUN_TEST(test_decode_char_acceptance_is_exhaustive);
  RUN_TEST(test_decode_ignores_input_past_hex_len);
  RUN_TEST(test_decode_writes_exactly_half_length);

  RUN_TEST(test_roundtrip_all_bytes);
  RUN_TEST(test_roundtrip_uppercased_hex);

  return TEST_SUMMARY();
}
