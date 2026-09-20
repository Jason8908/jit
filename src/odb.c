#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include "jit/compress.h"
#include "jit/file.h"
#include "jit/hash.h"
#include "jit/object.h"
#include "jit/odb.h"
#include "jit/shared.h"
#include "jit/strbuf.h"

#define OBJECT_MODE 0444

static void set_err(strbuf_t *err, const char *what, int e) {
  if (err)
    strbuf_catf(err, "%s: %s", what, strerror(e));
}

static bool object_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

int odb_write(oid_sha1_t *out, const char *objects_dir, obj_type_t type,
              const void *data, size_t len, strbuf_t *err) {
  strbuf_t encoded, compressed, dir, path;
  oid_path_t components;
  int result = -1;

  strbuf_init(&encoded, len + 32);
  strbuf_init(&compressed, 0);
  strbuf_init(&dir, 0);
  strbuf_init(&path, 0);

  object_hash_and_encode(out, &encoded, type, data, len);
  components = oid_to_components(*out);

  strbuf_catf(&dir, "%s/%s", objects_dir, components.prefix);
  strbuf_catf(&path, "%s/%s", dir.buf, components.rest);

  if (object_exists(strbuf_cstr(&path))) {
    result = 0;
    goto done;
  }

  if (!ensure_dir(strbuf_cstr(&dir))) {
    int e = errno;
    set_err(err, strbuf_cstr(&dir), e);
    goto done;
  }

  if (compress_deflate(&compressed, encoded.buf, encoded.len, err) < 0)
    goto done;

  if (file_write_atomic(&compressed, strbuf_cstr(&path), OBJECT_MODE, err) < 0)
    goto done;

  result = 0;

done:
  strbuf_release(&encoded);
  strbuf_release(&compressed);
  strbuf_release(&dir);
  strbuf_release(&path);
  return result;
}
