#ifndef JIT_TEST_TEMP_H
#define JIT_TEST_TEMP_H

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Temporary directory helpers for the suites that touch the filesystem.
 *
 * mkdtemp is hidden by the POSIX feature macro this project builds with, so
 * the directory name is built by hand from the pid and a counter.
 */

static void temp_remove(const char *path) {
  DIR *dir = opendir(path);

  if (dir != NULL) {
    const struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;

      char child[PATH_MAX];
      snprintf(child, sizeof child, "%s/%s", path, entry->d_name);
      temp_remove(child);
    }
    closedir(dir);
  }

  if (rmdir(path) < 0) unlink(path);
}

static void temp_root(char *buf, size_t size, const char *prefix) {
  static unsigned counter;

  snprintf(buf, size, "/tmp/jit-%s-%ld-%u", prefix, (long)getpid(), counter++);
  temp_remove(buf);
  if (mkdir(buf, 0700) < 0) abort();
}

#endif
