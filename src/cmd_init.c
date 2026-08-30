#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jit/builtin.h"
#include "jit/parse_options.h"
#include "jit/usage.h"
#include "jit/shared.h"

#define JIT_DIR ".jit"

static const char *const init_usage[] = {
  "jit init",
  NULL
};


static const char *const repo_dirs[] = {
  JIT_DIR,
  JIT_DIR "/objects",
};

static bool ensure_dir(const char *path);

int cmd_init(int argc, const char **argv) {
  const option_t options[] = { OPT_END() };

  struct stat st;
  bool already_initialized = (stat(JIT_DIR, &st) == 0) && S_ISDIR(st.st_mode);

  argc = parse_options(argc, argv, options, init_usage);

  if (argc != 0) 
    warning("jit init does not take any arguments, ignoring them");

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof cwd) == NULL)
    die("cannot determine current directory: %s", strerror(errno));

  for (size_t i = 0; i < ARRAY_SIZE(repo_dirs); i++) {
    if (!ensure_dir(repo_dirs[i]))
      die("cannot create '%s': %s", repo_dirs[i], strerror(errno));
  }

  if (already_initialized)
    printf("Reinitialized existing Jit repository in %s\n", strcat(cwd, "/" JIT_DIR));
  else
    printf("Initialized empty Jit repository in %s\n", strcat(cwd, "/" JIT_DIR));
  
  return 0;
}

static bool ensure_dir(const char *path) {
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