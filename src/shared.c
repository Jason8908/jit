#include "jit/shared.h"
#include <errno.h>
#include <sys/stat.h>

bool is_jit_repository(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool ensure_dir(const char *path) {
  if (mkdir(path, 0777) == 0)
    return true;

  if (errno != EEXIST)
    return false;

  struct stat st;
  if (stat(path, &st) < 0)
    return false;

  if (!S_ISDIR(st.st_mode)) {
    errno = ENOTDIR;
    return false;
  }

  return true;
}