#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include "jit/dir.h"
#include "jit/shared.h"
#include "jit/strbuf.h"
#include "jit/usage.h"

static const char *const skip_names[] = { JIT_DIR, GIT_DIR };

typedef struct walk_ctx {
  dir_walk_fn fn;
  void *data;
  strbuf_t *err;
  int had_error;
} walk_ctx_t;

static void set_err(strbuf_t *err, const char *what, int e) {
  if (err)
    strbuf_catf(err, "%s: %s", what, strerror(e));
}

static bool is_skipped(const char *name) {
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
    return true;

  for (size_t i = 0; i < ARRAY_SIZE(skip_names); i++) {
    if (strcmp(name, skip_names[i]) == 0)
      return true;
  }

  return false;
}

static int read_names(const char *path, strbuf_t *names) {
  DIR *dir = opendir(path);
  if (dir == NULL)
    return -1;

  const struct dirent *entry;
  errno = 0;
  while ((entry = readdir(dir)) != NULL) {
    if (!is_skipped(entry->d_name))
      strbuf_cat(names, entry->d_name, strlen(entry->d_name) + 1);
    errno = 0;
  }

  int e = errno;
  closedir(dir);

  if (e != 0) {
    errno = e;
    return -1;
  }

  return 0;
}

static int walk(strbuf_t *path, walk_ctx_t *ctx, int depth) {
  strbuf_t names;
  int result = 0;

  strbuf_init(&names, 0);

  if (read_names(strbuf_cstr(path), &names) < 0) {
    int e = errno;
    strbuf_release(&names);

    if (depth == 0) {
      set_err(ctx->err, strbuf_cstr(path), e);
      return -1;
    }

    warning("cannot read '%s': %s", strbuf_cstr(path), strerror(e));
    ctx->had_error = 1;
    return 0;
  }

  size_t saved = path->len;

  for (size_t i = 0; i < names.len; i += strlen(names.buf + i) + 1) {
    strbuf_catf(path, "/%s", names.buf + i);

    const char *entry = strbuf_cstr(path);
    struct stat st;

    if (lstat(entry, &st) < 0) {
      warning("cannot stat '%s': %s", entry, strerror(errno));
      ctx->had_error = 1;
    } else if (S_ISDIR(st.st_mode)) {
      result = walk(path, ctx, depth + 1);
    } else if (S_ISREG(st.st_mode)) {
      result = ctx->fn(entry, ctx->data);
    } else if (S_ISLNK(st.st_mode)) {
      warning("skipping symlink '%s'", entry);
    }

    strbuf_truncate(path, saved);

    if (result != 0)
      break;
  }

  strbuf_release(&names);
  return result;
}

int dir_walk(const char *root, dir_walk_fn fn, void *data, strbuf_t *err) {
  struct stat st;
  if (lstat(root, &st) < 0) {
    set_err(err, root, errno);
    return -1;
  }

  if (!S_ISDIR(st.st_mode)) {
    set_err(err, root, ENOTDIR);
    return -1;
  }

  walk_ctx_t ctx = { .fn = fn, .data = data, .err = err, .had_error = 0 };
  strbuf_t path;

  strbuf_init(&path, 0);
  strbuf_catf(&path, "%s", root);

  while (path.len > 1 && path.buf[path.len - 1] == '/')
    strbuf_truncate(&path, path.len - 1);

  int result = walk(&path, &ctx, 0);
  strbuf_release(&path);

  if (result == 0 && ctx.had_error) {
    if (err)
      strbuf_catf(err, "some entries could not be read");
    return -1;
  }

  return result;
}
