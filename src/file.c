#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jit/file.h"
#include "jit/strbuf.h"

#define TEMP_SUFFIX "tmp_obj_XXXXXX"

static void set_err(strbuf_t *err, const char *what, int e) {
  if (err)
    strbuf_catf(err, "%s: %s", what, strerror(e));
}

/**
 * Writes all of `len` bytes, resuming after a signal.
 *
 * A single write() may transfer fewer bytes than asked for; treating that
 * as success is how a file ends up truncated under a name that claims
 * otherwise.
 */
static int write_all(int fd, const char *data, size_t len) {
  while (len > 0) {
    ssize_t written = write(fd, data, len);

    if (written < 0) {
      if (errno == EINTR) continue;
      return -1;
    }

    if (written == 0) {
      errno = EIO;
      return -1;
    }

    data += written;
    len -= (size_t)written;
  }

  return 0;
}

/**
 * Builds the temporary file template, in the same directory as `path`.
 *
 * Same directory because rename is only atomic within a filesystem, and
 * separate from the final path because mkstemp rewrites its argument.
 */
static void build_template(strbuf_t *tmp, const char *path) {
  const char *slash = strrchr(path, '/');

  if (slash != NULL)
    strbuf_cat(tmp, path, (size_t)(slash - path) + 1);

  strbuf_cat(tmp, TEMP_SUFFIX, sizeof TEMP_SUFFIX - 1);
  strbuf_cstr(tmp);
}

int file_write_atomic(const strbuf_t *buf, const char *path, mode_t mode, strbuf_t *err) {
  strbuf_t tmp;
  int fd, result = -1, renamed = 0;

  strbuf_init(&tmp, 0);
  build_template(&tmp, path);

  fd = mkstemp(tmp.buf);
  if (fd < 0) {
    set_err(err, "mkstemp", errno);
    strbuf_release(&tmp);
    return -1;
  }

  if (buf->len > 0 && write_all(fd, buf->buf, buf->len) < 0) {
    set_err(err, "write", errno);
    close(fd);
    goto done;
  }

  if (fchmod(fd, mode) < 0) {
    set_err(err, "fchmod", errno);
    close(fd);
    goto done;
  }

  if (close(fd) < 0) {
    set_err(err, "close", errno);
    goto done;
  }

  if (rename(tmp.buf, path) < 0) {
    set_err(err, "rename", errno);
    goto done;
  }

  renamed = 1;
  result = 0;

done:
  if (!renamed)
    (void)unlink(tmp.buf);

  strbuf_release(&tmp);
  return result;
}
