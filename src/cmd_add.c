#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include "jit/builtin.h"
#include "jit/hash.h"
#include "jit/object.h"
#include "jit/blob.h"
#include "jit/dir.h"
#include "jit/odb.h"
#include "jit/parse_options.h"
#include "jit/strbuf.h"
#include "jit/usage.h"
#include "jit/shared.h"

static const char *const add_usage[] = {
  "jit add <file|directory>...",
  NULL
};

static int add_file(const char *path) {
  strbuf_t raw, err;
  oid_sha1_t oid;
  int result;

  strbuf_init(&raw, 0);
  strbuf_init(&err, 0);

  if (blob_read_path(&raw, path, &err) < 0)
    result = error("failed to read file '%s': %s", path, strbuf_cstr(&err));
  else if (odb_write(&oid, JIT_OBJECTS_DIR, OBJ_BLOB, raw.buf, raw.len, &err) < 0)
    result = error("failed to write object for '%s': %s", path, strbuf_cstr(&err));
  else
    result = 0;

  strbuf_release(&raw);
  strbuf_release(&err);
  return result;
}

static int add_walk_entry(const char *path, void *data) {
  int *failed = data;

  if (add_file(path) < 0)
    *failed = 1;

  return 0;
}

static int add_directory(const char *path) {
  strbuf_t err;
  int failed = 0;

  strbuf_init(&err, 0);

  if (dir_walk(path, add_walk_entry, &failed, &err) != 0)
    failed = error("cannot add '%s': %s", path, strbuf_cstr(&err));

  strbuf_release(&err);
  return failed ? -1 : 0;
}

/**
 * True if `path` names the repository directory or something inside it.
 *
 * dir_walk skips the repository wherever it meets it as an entry, but it
 * cannot check its own root. Walking the object store would hash objects
 * into the very directories being read, and readdir promises nothing about
 * entries created during a scan.
 */
static bool is_inside_repo_dir(const char *path) {
  while (strncmp(path, "./", 2) == 0)
    path += 2;

  size_t len = strlen(JIT_DIR);
  if (strncmp(path, JIT_DIR, len) != 0)
    return false;

  return path[len] == '\0' || path[len] == '/';
}

static int add_path(const char *path) {
  struct stat st;

  if (is_inside_repo_dir(path))
    return error("'%s' is inside the repository directory", path);

  if (lstat(path, &st) < 0)
    return error("cannot stat '%s': %s", path, strerror(errno));

  if (S_ISDIR(st.st_mode))
    return add_directory(path);

  if (S_ISREG(st.st_mode))
    return add_file(path);

  return error("'%s' is not a regular file", path);
}

int cmd_add(int argc, const char **argv) {
  const option_t options[] = { OPT_END() };
  int failed = 0;

  argc = parse_options(argc, argv, options, add_usage);

  if (!is_jit_repository(JIT_DIR))
    die("not a jit repository");

  if (argc == 0)
    usage_with_options(add_usage, options);

  for (int i = 0; i < argc; i++)
    failed |= add_path(argv[i]) < 0;

  return failed;
}
