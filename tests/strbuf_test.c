#include <string.h>
#include "jit/strbuf.h"
#include "test.h"

// init / release

static void test_init_zero_allocates_nothing(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  ASSERT(sb.buf == NULL);
  ASSERT(sb.len == 0);
  ASSERT(sb.alloc == 0);

  strbuf_release(&sb);
}

static void test_init_reserves_requested_size(void) {
  strbuf_t sb;
  strbuf_init(&sb, 100);

  ASSERT(sb.buf != NULL);
  ASSERT(sb.len == 0);
  ASSERT(sb.alloc >= 100);

  strbuf_release(&sb);
}

static void test_release_resets_fields(void) {
  strbuf_t sb;
  strbuf_init(&sb, 32);
  strbuf_cat(&sb, "data", 4);

  strbuf_release(&sb);

  ASSERT(sb.buf == NULL);
  ASSERT(sb.len == 0);
  ASSERT(sb.alloc == 0);
}

static void test_release_is_idempotent(void) {
  strbuf_t sb;
  strbuf_init(&sb, 32);

  strbuf_release(&sb);
  strbuf_release(&sb);

  ASSERT(sb.buf == NULL);
  ASSERT(sb.alloc == 0);
}

// grow

static void test_grow_reserves_free_space(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_grow(&sb, 10);

  ASSERT(sb.len == 0);
  ASSERT(sb.alloc - sb.len >= 10);
  ASSERT(sb.buf != NULL);

  strbuf_release(&sb);
}

static void test_grow_is_noop_when_space_available(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);
  strbuf_grow(&sb, 100);

  size_t alloc_before = sb.alloc;
  char *buf_before = sb.buf;
  strbuf_grow(&sb, 10);

  ASSERT(sb.alloc == alloc_before);
  ASSERT(sb.buf == buf_before);

  strbuf_release(&sb);
}

static void test_grow_accounts_for_existing_content(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);
  strbuf_cat(&sb, "hello", 5);

  strbuf_grow(&sb, 50);

  ASSERT(sb.len == 5);
  ASSERT(sb.alloc - sb.len >= 50);
  ASSERT(memcmp(sb.buf, "hello", 5) == 0);

  strbuf_release(&sb);
}

// cat

static void test_cat_into_empty_buffer(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_cat(&sb, "hello", 5);

  ASSERT(sb.len == 5);
  ASSERT(sb.alloc >= sb.len);
  ASSERT(memcmp(sb.buf, "hello", 5) == 0);

  strbuf_release(&sb);
}

static void test_cat_appends_across_reallocs(void) {
  strbuf_t sb;
  char expected[161];
  int i;

  strbuf_init(&sb, 0);
  expected[0] = '\0';

  for (i = 0; i < 20; i++) {
    strbuf_cat(&sb, "abcdefgh", 8);
    memcpy(expected + (size_t)i * 8, "abcdefgh", 8);
  }
  expected[160] = '\0';

  ASSERT(sb.len == 160);
  ASSERT(sb.alloc >= sb.len);
  ASSERT(memcmp(sb.buf, expected, 160) == 0);

  strbuf_release(&sb);
}

static void test_cat_stores_embedded_nul_bytes(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_cat(&sb, "a\0b", 3);

  ASSERT(sb.len == 3);
  ASSERT(memcmp(sb.buf, "a\0b", 3) == 0);

  strbuf_release(&sb);
}

static void test_cat_of_zero_bytes_is_noop(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_cat(&sb, "", 0);

  ASSERT(sb.len == 0);

  strbuf_cat(&sb, "xy", 2);
  strbuf_cat(&sb, "", 0);

  ASSERT(sb.len == 2);
  ASSERT(memcmp(sb.buf, "xy", 2) == 0);

  strbuf_release(&sb);
}

// catf

static void test_catf_formats_arguments(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_catf(&sb, "%s=%d", "x", 42);

  ASSERT(sb.len == 4);
  ASSERT(memcmp(sb.buf, "x=42", 4) == 0);

  strbuf_release(&sb);
}

static void test_catf_excludes_terminator_from_len(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_catf(&sb, "hello");

  ASSERT(sb.len == strlen("hello"));
  ASSERT(sb.alloc > sb.len);
  ASSERT(sb.buf[sb.len] == '\0');

  strbuf_release(&sb);
}

static void test_catf_overwrites_previous_terminator(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_cat(&sb, "ab", 2);
  strbuf_catf(&sb, "%d", 7);

  ASSERT(sb.len == 3);
  ASSERT(memcmp(sb.buf, "ab7", 3) == 0);

  strbuf_catf(&sb, "%d", 8);

  ASSERT(sb.len == 4);
  ASSERT(memcmp(sb.buf, "ab78", 4) == 0);

  strbuf_release(&sb);
}

static void test_catf_with_empty_result(void) {
  strbuf_t sb;
  strbuf_init(&sb, 0);

  strbuf_cat(&sb, "xy", 2);
  strbuf_catf(&sb, "%s", "");

  ASSERT(sb.len == 2);
  ASSERT(memcmp(sb.buf, "xy", 2) == 0);

  strbuf_release(&sb);
}

static void test_catf_grows_for_large_output(void) {
  strbuf_t sb;
  char big[1001];

  memset(big, 'z', 1000);
  big[1000] = '\0';

  strbuf_init(&sb, 0);
  strbuf_catf(&sb, "%s", big);

  ASSERT(sb.len == 1000);
  ASSERT(sb.alloc >= sb.len);
  ASSERT(memcmp(sb.buf, big, 1000) == 0);

  strbuf_release(&sb);
}

int main(void) {
  RUN_TEST(test_init_zero_allocates_nothing);
  RUN_TEST(test_init_reserves_requested_size);
  RUN_TEST(test_release_resets_fields);
  RUN_TEST(test_release_is_idempotent);

  RUN_TEST(test_grow_reserves_free_space);
  RUN_TEST(test_grow_is_noop_when_space_available);
  RUN_TEST(test_grow_accounts_for_existing_content);

  RUN_TEST(test_cat_into_empty_buffer);
  RUN_TEST(test_cat_appends_across_reallocs);
  RUN_TEST(test_cat_stores_embedded_nul_bytes);
  RUN_TEST(test_cat_of_zero_bytes_is_noop);

  RUN_TEST(test_catf_formats_arguments);
  RUN_TEST(test_catf_excludes_terminator_from_len);
  RUN_TEST(test_catf_overwrites_previous_terminator);
  RUN_TEST(test_catf_with_empty_result);
  RUN_TEST(test_catf_grows_for_large_output);

  return TEST_SUMMARY();
}
