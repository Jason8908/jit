#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "jit/compress.h"
#include "jit/strbuf.h"
#include "test.h"

/**
 * Streams produced by real git 2.50.1 at compression level 1, for the
 * encoded form of each object. Inputs are written out as literals rather
 * than built with object_encode so that a failure here means compression
 * is wrong, not encoding.
 */
static const unsigned char EMPTY_BLOB_Z[] = {
  0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x60, 0x00, 0x00, 0x09,
  0xb0, 0x01, 0xf0
};

static const unsigned char HELLO_WORLD_Z[] = {
  0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x34, 0x64, 0xc8, 0x48,
  0xcd, 0xc9, 0xc9, 0x57, 0x28, 0xcf, 0x2f, 0xca, 0x49, 0x01, 0x00, 0x3d,
  0x7b, 0x06, 0x7e
};

static const unsigned char HELLO_NL_Z[] = {
  0x78, 0x01, 0x4b, 0xca, 0xc9, 0x4f, 0x52, 0x30, 0x63, 0xc8, 0x48, 0xcd,
  0xc9, 0xc9, 0xe7, 0x02, 0x00, 0x1d, 0xc5, 0x04, 0x14
};

/**
 * Deterministic pseudo-random bytes.
 *
 * A fixed LCG rather than rand() so that a failure reproduces exactly.
 */
static void fill_incompressible(unsigned char *buf, size_t len) {
  unsigned int state = 0x12345678u;

  for (size_t i = 0; i < len; i++) {
    state = state * 1103515245u + 12345u;
    buf[i] = (unsigned char)(state >> 16);
  }
}

static int deflates_to(const void *data, size_t len, const void *expected, size_t expected_len) {
  strbuf_t out;
  int ok;

  strbuf_init(&out, 0);
  ok = compress_deflate(&out, data, len, NULL) == 0 &&
       out.len == expected_len &&
       memcmp(out.buf, expected, expected_len) == 0;

  strbuf_release(&out);
  return ok;
}

// git compatibility

static void test_matches_git_empty_blob(void) {
  ASSERT(deflates_to("blob 0\0", 7, EMPTY_BLOB_Z, sizeof EMPTY_BLOB_Z));
}

static void test_matches_git_hello_world(void) {
  ASSERT(deflates_to("blob 11\0hello world", 19, HELLO_WORLD_Z, sizeof HELLO_WORLD_Z));
}

static void test_matches_git_hello_newline(void) {
  ASSERT(deflates_to("blob 6\0hello\n", 13, HELLO_NL_Z, sizeof HELLO_NL_Z));
}

static void test_header_advertises_level_1(void) {
  unsigned char raw[64 * 1024];
  strbuf_t out;
  int ok;

  fill_incompressible(raw, sizeof raw);
  strbuf_init(&out, 0);

  ok = compress_deflate(&out, raw, sizeof raw, NULL) == 0 &&
       out.len >= 2 &&
       (unsigned char)out.buf[0] == 0x78 &&
       (unsigned char)out.buf[1] == 0x01;

  strbuf_release(&out);

  ASSERT(ok);
}

// buffer handling

static void test_appends_to_existing_content(void) {
  strbuf_t out;
  int ok;

  strbuf_init(&out, 0);
  strbuf_cat(&out, "XY", 2);

  ok = compress_deflate(&out, "blob 6\0hello\n", 13, NULL) == 0 &&
       out.len == 2 + sizeof HELLO_NL_Z &&
       memcmp(out.buf, "XY", 2) == 0 &&
       memcmp(out.buf + 2, HELLO_NL_Z, sizeof HELLO_NL_Z) == 0;

  strbuf_release(&out);

  ASSERT(ok);
}

static void test_two_streams_concatenate(void) {
  strbuf_t out;
  int ok;

  strbuf_init(&out, 0);

  ok = compress_deflate(&out, "blob 0\0", 7, NULL) == 0 &&
       compress_deflate(&out, "blob 6\0hello\n", 13, NULL) == 0 &&
       out.len == sizeof EMPTY_BLOB_Z + sizeof HELLO_NL_Z &&
       memcmp(out.buf, EMPTY_BLOB_Z, sizeof EMPTY_BLOB_Z) == 0 &&
       memcmp(out.buf + sizeof EMPTY_BLOB_Z, HELLO_NL_Z, sizeof HELLO_NL_Z) == 0;

  strbuf_release(&out);

  ASSERT(ok);
}

/**
 * High-entropy input deflates to more bytes than it started with, so this
 * is what catches sizing the destination by input length instead of
 * compressBound.
 */
static void test_incompressible_input_does_not_overflow(void) {
  const size_t len = 1024 * 1024;
  unsigned char *raw = malloc(len);
  strbuf_t out;
  int ok;

  if (raw == NULL) abort();
  fill_incompressible(raw, len);

  strbuf_init(&out, 0);
  ok = compress_deflate(&out, raw, len, NULL) == 0 && out.len > len;

  strbuf_release(&out);
  free(raw);

  ASSERT(ok);
}

// edge cases

static void test_empty_input_produces_a_valid_stream(void) {
  strbuf_t out;
  int ok;

  strbuf_init(&out, 0);
  ok = compress_deflate(&out, "", 0, NULL) == 0 &&
       out.len >= 8 &&
       (unsigned char)out.buf[0] == 0x78 &&
       (unsigned char)out.buf[1] == 0x01;

  strbuf_release(&out);

  ASSERT(ok);
}

static void test_null_data_with_zero_length(void) {
  strbuf_t out;
  int ok;

  strbuf_init(&out, 0);
  ok = compress_deflate(&out, NULL, 0, NULL) == 0 && out.len >= 8;

  strbuf_release(&out);

  ASSERT(ok);
}

static void test_roundtrips_through_uncompress(void) {
  const size_t len = 64 * 1024;
  unsigned char *raw = malloc(len);
  unsigned char *back = malloc(len);
  strbuf_t out;
  uLongf back_len = (uLongf)len;
  int ok;

  if (raw == NULL || back == NULL) abort();
  fill_incompressible(raw, len);

  strbuf_init(&out, 0);
  ok = compress_deflate(&out, raw, len, NULL) == 0 &&
       uncompress(back, &back_len, (const Bytef *)out.buf, (uLong)out.len) == Z_OK &&
       back_len == len &&
       memcmp(raw, back, len) == 0;

  strbuf_release(&out);
  free(raw);
  free(back);

  ASSERT(ok);
}

static void test_embedded_nul_bytes_survive(void) {
  static const unsigned char payload[] = { 'a', 0x00, 'b', 0x00, 0x00, 'c' };
  unsigned char back[sizeof payload];
  strbuf_t out;
  uLongf back_len = (uLongf)sizeof back;
  int ok;

  strbuf_init(&out, 0);
  ok = compress_deflate(&out, payload, sizeof payload, NULL) == 0 &&
       uncompress(back, &back_len, (const Bytef *)out.buf, (uLong)out.len) == Z_OK &&
       back_len == sizeof payload &&
       memcmp(payload, back, sizeof payload) == 0;

  strbuf_release(&out);

  ASSERT(ok);
}

static void test_a_null_err_is_accepted(void) {
  strbuf_t out;
  int ok;

  strbuf_init(&out, 0);
  ok = compress_deflate(&out, "blob 0\0", 7, NULL) == 0;
  strbuf_release(&out);

  ASSERT(ok);
}

static void test_an_err_buffer_is_left_alone_on_success(void) {
  strbuf_t out, err;
  int ok;

  strbuf_init(&out, 0);
  strbuf_init(&err, 0);

  ok = compress_deflate(&out, "blob 0\0", 7, &err) == 0 && err.len == 0;

  strbuf_release(&out);
  strbuf_release(&err);

  ASSERT(ok);
}

int main(void) {
  RUN_TEST(test_matches_git_empty_blob);
  RUN_TEST(test_matches_git_hello_world);
  RUN_TEST(test_matches_git_hello_newline);
  RUN_TEST(test_header_advertises_level_1);

  RUN_TEST(test_appends_to_existing_content);
  RUN_TEST(test_two_streams_concatenate);
  RUN_TEST(test_incompressible_input_does_not_overflow);

  RUN_TEST(test_empty_input_produces_a_valid_stream);
  RUN_TEST(test_null_data_with_zero_length);
  RUN_TEST(test_roundtrips_through_uncompress);
  RUN_TEST(test_embedded_nul_bytes_survive);

  RUN_TEST(test_a_null_err_is_accepted);
  RUN_TEST(test_an_err_buffer_is_left_alone_on_success);

  return TEST_SUMMARY();
}
