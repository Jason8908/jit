#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "jit/blob.h"
#include "jit/object.h"
#include "jit/strbuf.h"

static const int CHUNK_SIZE = 4096;

static void set_err(strbuf_t *err, int e) {
  if (err)
    strbuf_catf(err, "%s", strerror(e));
}

int blob_read_stream(strbuf_t *payload, FILE *fp, strbuf_t *err) {
  char chunk[CHUNK_SIZE];
  size_t n;
  while ((n = fread(chunk, 1, CHUNK_SIZE, fp)) > 0)
    strbuf_cat(payload, chunk, n);

  if (ferror(fp)) {
    set_err(err, errno);
    return -1;
  }
  return 0;
}

int blob_read_path(strbuf_t *payload, const char *path, strbuf_t *err) {
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
    set_err(err, errno);
    return -1;
  }

  int ret = blob_read_stream(payload, fp, err);
  fclose(fp);

  return ret;
}

int blob_hash_path(oid_sha1_t *out, const char *path, strbuf_t *err) {
  strbuf_t payload;

  strbuf_init(&payload, 0);
  if (blob_read_path(&payload, path, err) < 0) {
    strbuf_release(&payload);
    return ERR_FAIL_TO_READ_FILE;
  }

  object_hash(out, OBJ_BLOB, payload.buf, payload.len);
  strbuf_release(&payload);
  return 0;
}

int blob_hash_stream(oid_sha1_t *out, FILE *fp, strbuf_t *err) {
  strbuf_t payload;

  strbuf_init(&payload, 0);
  if (blob_read_stream(&payload, fp, err) < 0) {
    strbuf_release(&payload);
    return ERR_FAIL_TO_READ_FILE;
  }

  object_hash(out, OBJ_BLOB, payload.buf, payload.len);
  strbuf_release(&payload);
  return 0;
}