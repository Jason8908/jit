#include <zlib.h>

#include "jit/compress.h"
#include "jit/strbuf.h"

#define DEFLATE_LEVEL 1

int compress_deflate(strbuf_t *out, const void *data, size_t len, strbuf_t *err) {
  if (data == NULL)
    data = "";

  uLong bound = compressBound((uLong)len);

  strbuf_grow(out, (size_t)bound);

  uLongf dest_len = bound;
  int rc = compress2((Bytef *)(out->buf + out->len), &dest_len,
                     (const Bytef *)data, (uLong)len, DEFLATE_LEVEL);

  if (rc != Z_OK) {
    if (err)
      strbuf_catf(err, "zlib compression failed (rc=%d)", rc);
    return -1;
  }

  out->len += (size_t)dest_len;
  return 0;
}
