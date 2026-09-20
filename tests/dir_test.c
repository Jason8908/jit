#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jit/dir.h"
#include "jit/strbuf.h"
#include "temp.h"
#include "test.h"

#define MAX_PATHS 300
#define MAX_PATH_LEN 256

// scaffolding

static char root[PATH_MAX];

/**
 * Paths reported by the walk, relative to the root.
 *
 * File-static rather than a local so that a failing ASSERT, which returns
 * from the test immediately, cannot leak it.
 */
static struct {
  char paths[MAX_PATHS][MAX_PATH_LEN];
  size_t count;
  int stop_at;
} seen;

static void make_root(void) {
  temp_root(root, sizeof root, "dir");

  seen.count = 0;
  seen.stop_at = 0;
}

static const char *at(const char *rel) {
  static char path[PATH_MAX];
  snprintf(path, sizeof path, "%s/%s", root, rel);
  return path;
}

static void make_dir(const char *rel) {
  if (mkdir(at(rel), 0777) < 0 && errno != EEXIST) abort();
}

static void make_file(const char *rel) {
  FILE *fp = fopen(at(rel), "w");
  if (fp == NULL) abort();
  fputs("contents\n", fp);
  fclose(fp);
}

static void make_link(const char *target, const char *rel) {
  char path[PATH_MAX];
  snprintf(path, sizeof path, "%s/%s", root, rel);
  if (symlink(target, path) < 0) abort();
}

static int collect(const char *path, void *data) {
  int *calls = data;
  size_t prefix = strlen(root) + 1;

  (*calls)++;

  if (seen.count < MAX_PATHS && strlen(path) > prefix)
    snprintf(seen.paths[seen.count++], MAX_PATH_LEN, "%s", path + prefix);

  if (seen.stop_at != 0 && *calls >= seen.stop_at)
    return 7;

  return 0;
}

static int cmp_paths(const void *a, const void *b) {
  return strcmp(a, b);
}

/**
 * The reported paths, sorted and space-separated.
 *
 * Sorting matters: readdir order is filesystem-dependent, so comparing
 * unsorted output passes on APFS and fails on ext4.
 */
static const char *reported(void) {
  static char out[MAX_PATHS * 32];
  size_t len = 0;

  qsort(seen.paths, seen.count, MAX_PATH_LEN, cmp_paths);
  out[0] = '\0';

  for (size_t i = 0; i < seen.count && len < sizeof out - 1; i++)
    len += (size_t)snprintf(out + len, sizeof out - len, "%s%s", i ? " " : "", seen.paths[i]);

  return out;
}

static bool was_reported(const char *rel) {
  for (size_t i = 0; i < seen.count; i++) {
    if (strcmp(seen.paths[i], rel) == 0) return true;
  }
  return false;
}

/**
 * Walks the root, tears the tree down, and leaves the results in `seen`.
 *
 * Teardown happens here so that no test has cleanup sitting below an
 * ASSERT, which would be skipped on failure.
 */
static int walk_and_teardown(void) {
  strbuf_t err;
  int calls = 0;
  int result;

  strbuf_init(&err, 0);
  result = dir_walk(root, collect, &calls, &err);
  strbuf_release(&err);
  temp_remove(root);
  return result;
}

// visiting files

static void test_reports_every_file_in_a_flat_directory(void) {
  make_root();
  make_file("a.txt");
  make_file("b.txt");
  make_file("c.txt");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "a.txt b.txt c.txt") == 0);
}

static void test_descends_into_nested_directories(void) {
  make_root();
  make_file("top.txt");
  make_dir("a");
  make_file("a/one.txt");
  make_dir("a/b");
  make_file("a/b/two.txt");
  make_dir("a/b/c");
  make_file("a/b/c/three.txt");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "a/b/c/three.txt a/b/two.txt a/one.txt top.txt") == 0);
}

static void test_empty_directory_reports_nothing(void) {
  make_root();

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(seen.count == 0);
}

static void test_directories_without_files_report_nothing(void) {
  make_root();
  make_dir("a");
  make_dir("a/b");
  make_dir("c");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(seen.count == 0);
}

static void test_dotfiles_are_reported(void) {
  make_root();
  make_file(".hidden");
  make_dir(".config");
  make_file(".config/settings");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), ".config/settings .hidden") == 0);
}

// the repository directory

static void test_skips_the_repository_directory(void) {
  make_root();
  make_file("kept.txt");
  make_dir(".jit");
  make_dir(".jit/objects");
  make_file(".jit/objects/stored");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "kept.txt") == 0);
}

static void test_skips_a_nested_repository_directory(void) {
  make_root();
  make_dir("a");
  make_dir("a/b");
  make_file("a/b/kept.txt");
  make_dir("a/b/.jit");
  make_file("a/b/.jit/stored");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "a/b/kept.txt") == 0);
}

static void test_skips_a_regular_file_named_like_the_repository(void) {
  make_root();
  make_file(".jit");
  make_file("kept.txt");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "kept.txt") == 0);
}

// entries that are not regular files

static void test_does_not_report_a_symlink_to_a_file(void) {
  make_root();
  make_file("real.txt");
  make_link("real.txt", "link.txt");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "real.txt") == 0);
}

static void test_does_not_descend_a_symlink_to_a_directory(void) {
  make_root();
  make_dir("real");
  make_file("real/behind.txt");
  make_link("real", "link");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "real/behind.txt") == 0);
}

static void test_a_symlink_loop_terminates(void) {
  make_root();
  make_file("real.txt");
  make_link("b", "a");
  make_link("a", "b");
  make_dir("d");
  make_link("../d", "d/self");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "real.txt") == 0);
}

static void test_does_not_report_a_fifo(void) {
  make_root();
  make_file("real.txt");
  if (mkfifo(at("pipe"), 0666) < 0) abort();

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(strcmp(reported(), "real.txt") == 0);
}

// the callback

static void test_a_non_zero_callback_stops_the_walk(void) {
  make_root();
  make_file("a.txt");
  make_file("b.txt");
  make_file("c.txt");
  seen.stop_at = 2;

  int result = walk_and_teardown();

  ASSERT(result == 7);
  ASSERT(seen.count == 2);
}

// failure paths

static void test_a_missing_root_fails(void) {
  strbuf_t err;
  int calls = 0;

  strbuf_init(&err, 0);
  int result = dir_walk("/tmp/jit-dir-does-not-exist", collect, &calls, &err);
  size_t err_len = err.len;
  strbuf_release(&err);

  ASSERT(result == -1);
  ASSERT(err_len > 0);
  ASSERT(calls == 0);
}

static void test_a_root_that_is_a_regular_file_fails(void) {
  make_root();
  make_file("plain.txt");

  strbuf_t err;
  int calls = 0;

  strbuf_init(&err, 0);
  int result = dir_walk(at("plain.txt"), collect, &calls, &err);
  size_t err_len = err.len;
  strbuf_release(&err);
  temp_remove(root);

  ASSERT(result == -1);
  ASSERT(err_len > 0);
  ASSERT(calls == 0);
}

static void test_a_null_err_is_accepted(void) {
  make_root();
  make_file("a.txt");

  int calls = 0;
  int ok = dir_walk(root, collect, &calls, NULL);
  int missing = dir_walk("/tmp/jit-dir-does-not-exist", collect, &calls, NULL);

  temp_remove(root);

  ASSERT(ok == 0);
  ASSERT(missing == -1);
  ASSERT(calls == 1);
}

static void test_an_unreadable_directory_is_skipped_but_the_walk_continues(void) {
  make_root();
  make_file("sibling.txt");
  make_dir("locked");
  make_file("locked/inside.txt");
  make_dir("after");
  make_file("after/later.txt");

  if (chmod(at("locked"), 0) < 0) abort();

  strbuf_t err;
  int calls = 0;

  strbuf_init(&err, 0);
  int result = dir_walk(root, collect, &calls, &err);
  size_t err_len = err.len;
  strbuf_release(&err);

  if (chmod(at("locked"), 0700) < 0) abort();
  temp_remove(root);

  ASSERT(result == -1);
  ASSERT(err_len > 0);
  ASSERT(was_reported("sibling.txt"));
  ASSERT(was_reported("after/later.txt"));
  ASSERT(!was_reported("locked/inside.txt"));
}

// path building

static void test_deep_nesting_builds_the_right_path(void) {
  const int depth = 60;
  char expected[MAX_PATH_LEN];
  char rel[MAX_PATH_LEN];
  size_t len = 0;

  make_root();
  rel[0] = '\0';

  for (int i = 0; i < depth; i++) {
    len += (size_t)snprintf(rel + len, sizeof rel - len, i ? "/d" : "d");
    make_dir(rel);
  }

  snprintf(expected, sizeof expected, "%s/bottom.txt", rel);
  make_file(expected);

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(seen.count == 1);
  ASSERT(strcmp(reported(), expected) == 0);
}

static void test_many_entries_survive_buffer_reallocation(void) {
  const size_t count = 200;
  char rel[MAX_PATH_LEN];

  make_root();

  for (size_t i = 0; i < count; i++) {
    snprintf(rel, sizeof rel, "entry-%03zu-with-a-name-long-enough-to-force-regrowth", i);
    make_file(rel);
  }

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(seen.count == count);
  ASSERT(was_reported("entry-000-with-a-name-long-enough-to-force-regrowth"));
  ASSERT(was_reported("entry-199-with-a-name-long-enough-to-force-regrowth"));
}

static void test_awkward_file_names_are_reported_verbatim(void) {
  make_root();
  make_file("a space.txt");
  make_file("a\nnewline.txt");
  make_file("%s.txt");

  int result = walk_and_teardown();

  ASSERT(result == 0);
  ASSERT(seen.count == 3);
  ASSERT(was_reported("a space.txt"));
  ASSERT(was_reported("a\nnewline.txt"));
  ASSERT(was_reported("%s.txt"));
}

int main(void) {
  RUN_TEST(test_reports_every_file_in_a_flat_directory);
  RUN_TEST(test_descends_into_nested_directories);
  RUN_TEST(test_empty_directory_reports_nothing);
  RUN_TEST(test_directories_without_files_report_nothing);
  RUN_TEST(test_dotfiles_are_reported);

  RUN_TEST(test_skips_the_repository_directory);
  RUN_TEST(test_skips_a_nested_repository_directory);
  RUN_TEST(test_skips_a_regular_file_named_like_the_repository);

  RUN_TEST(test_does_not_report_a_symlink_to_a_file);
  RUN_TEST(test_does_not_descend_a_symlink_to_a_directory);
  RUN_TEST(test_a_symlink_loop_terminates);
  RUN_TEST(test_does_not_report_a_fifo);

  RUN_TEST(test_a_non_zero_callback_stops_the_walk);

  RUN_TEST(test_a_missing_root_fails);
  RUN_TEST(test_a_root_that_is_a_regular_file_fails);
  RUN_TEST(test_a_null_err_is_accepted);
  if (geteuid() != 0)
    RUN_TEST(test_an_unreadable_directory_is_skipped_but_the_walk_continues);

  RUN_TEST(test_deep_nesting_builds_the_right_path);
  RUN_TEST(test_many_entries_survive_buffer_reallocation);
  RUN_TEST(test_awkward_file_names_are_reported_verbatim);

  return TEST_SUMMARY();
}
