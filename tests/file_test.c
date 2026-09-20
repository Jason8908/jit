#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jit/file.h"
#include "jit/strbuf.h"
#include "temp.h"
#include "test.h"

static char root[PATH_MAX];

static const char *at(const char *rel) {
  static char path[PATH_MAX];
  snprintf(path, sizeof path, "%s/%s", root, rel);
  return path;
}

static int file_contents_are(const char *path, const void *expected, size_t len) {
  FILE *fp = fopen(path, "rb");
  char *got;
  int ok;

  if (fp == NULL) return 0;

  got = malloc(len + 1);
  if (got == NULL) abort();

  ok = fread(got, 1, len + 1, fp) == len && memcmp(got, expected, len) == 0;

  fclose(fp);
  free(got);
  return ok;
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

// writing

static void test_writes_contents_and_mode(void) {
  strbuf_t buf;
  int result;
  int contents_ok;
  mode_t mode;

  temp_root(root, sizeof root, "file");
  strbuf_init(&buf, 0);
  strbuf_cat(&buf, "hello", 5);

  result = file_write_atomic(&buf, at("out.txt"), 0444, NULL);
  contents_ok = file_contents_are(at("out.txt"), "hello", 5);
  mode = mode_of(at("out.txt"));

  strbuf_release(&buf);
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(contents_ok);
  ASSERT(mode == 0444);
}

static void test_writes_an_empty_buffer(void) {
  strbuf_t buf;
  int result;
  struct stat st;
  int stat_ok;

  temp_root(root, sizeof root, "file");
  strbuf_init(&buf, 0);

  result = file_write_atomic(&buf, at("empty.txt"), 0444, NULL);
  stat_ok = stat(at("empty.txt"), &st) == 0 && st.st_size == 0;

  strbuf_release(&buf);
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(stat_ok);
}

static void test_writes_a_buffer_spanning_many_writes(void) {
  const size_t len = 4 * 1024 * 1024;
  char *data = malloc(len);
  strbuf_t buf;
  int result, contents_ok;

  if (data == NULL) abort();
  memset(data, 'z', len);

  temp_root(root, sizeof root, "file");
  strbuf_init(&buf, 0);
  strbuf_cat(&buf, data, len);

  result = file_write_atomic(&buf, at("big.bin"), 0644, NULL);
  contents_ok = file_contents_are(at("big.bin"), data, len);

  strbuf_release(&buf);
  free(data);
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(contents_ok);
}

/**
 * The target of a rename may be read-only; only the containing directory's
 * permissions matter. Git relies on this to replace loose objects.
 */
static void test_replaces_an_existing_read_only_file(void) {
  strbuf_t first, second;
  int result, contents_ok;

  temp_root(root, sizeof root, "file");
  strbuf_init(&first, 0);
  strbuf_init(&second, 0);
  strbuf_cat(&first, "one", 3);
  strbuf_cat(&second, "two", 3);

  file_write_atomic(&first, at("f.txt"), 0444, NULL);
  result = file_write_atomic(&second, at("f.txt"), 0444, NULL);
  contents_ok = file_contents_are(at("f.txt"), "two", 3);

  strbuf_release(&first);
  strbuf_release(&second);
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(contents_ok);
}

// cleanup

static void test_leaves_no_temporary_behind_on_success(void) {
  strbuf_t buf;
  int result;
  size_t count;

  temp_root(root, sizeof root, "file");
  strbuf_init(&buf, 0);
  strbuf_cat(&buf, "hello", 5);

  result = file_write_atomic(&buf, at("out.txt"), 0444, NULL);
  count = entry_count(root);

  strbuf_release(&buf);
  temp_remove(root);

  ASSERT(result == 0);
  ASSERT(count == 1);
}

// failure paths

static void test_an_unwritable_directory_fails_and_leaves_nothing(void) {
  strbuf_t buf, err;
  int result;
  size_t err_len, count;

  temp_root(root, sizeof root, "file");
  strbuf_init(&buf, 0);
  strbuf_init(&err, 0);
  strbuf_cat(&buf, "hello", 5);

  if (chmod(root, 0500) < 0) abort();

  result = file_write_atomic(&buf, at("out.txt"), 0444, &err);
  err_len = err.len;
  count = entry_count(root);

  if (chmod(root, 0700) < 0) abort();
  strbuf_release(&buf);
  strbuf_release(&err);
  temp_remove(root);

  ASSERT(result == -1);
  ASSERT(err_len > 0);
  ASSERT(count == 0);
}

static void test_a_missing_directory_fails(void) {
  strbuf_t buf, err;
  int result;
  size_t err_len;

  temp_root(root, sizeof root, "file");
  strbuf_init(&buf, 0);
  strbuf_init(&err, 0);
  strbuf_cat(&buf, "hello", 5);

  result = file_write_atomic(&buf, at("no/such/dir/out.txt"), 0444, &err);
  err_len = err.len;

  strbuf_release(&buf);
  strbuf_release(&err);
  temp_remove(root);

  ASSERT(result == -1);
  ASSERT(err_len > 0);
}

static void test_a_null_err_is_accepted(void) {
  strbuf_t buf;
  int ok, failed;

  temp_root(root, sizeof root, "file");
  strbuf_init(&buf, 0);
  strbuf_cat(&buf, "hello", 5);

  ok = file_write_atomic(&buf, at("out.txt"), 0444, NULL);
  failed = file_write_atomic(&buf, at("no/such/dir/out.txt"), 0444, NULL);

  strbuf_release(&buf);
  temp_remove(root);

  ASSERT(ok == 0);
  ASSERT(failed == -1);
}

int main(void) {
  RUN_TEST(test_writes_contents_and_mode);
  RUN_TEST(test_writes_an_empty_buffer);
  RUN_TEST(test_writes_a_buffer_spanning_many_writes);
  RUN_TEST(test_replaces_an_existing_read_only_file);

  RUN_TEST(test_leaves_no_temporary_behind_on_success);

  if (geteuid() != 0)
    RUN_TEST(test_an_unwritable_directory_fails_and_leaves_nothing);
  RUN_TEST(test_a_missing_directory_fails);
  RUN_TEST(test_a_null_err_is_accepted);

  return TEST_SUMMARY();
}
