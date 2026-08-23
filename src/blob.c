#include <stdio.h>
#include "jit/blob.h"
#include "jit/strbuf.h"

static const int CHUNK_SIZE = 4096;

int blob_read_path(strbuf_t *payload, const char *path) {
  FILE *fp = fopen(path, "rb");
  if (fp == NULL) return -1;

  char chunk[CHUNK_SIZE];
  size_t n;
  while ((n = fread(chunk, 1, CHUNK_SIZE, fp)) > 0)
    strbuf_cat(payload, chunk, n);

  int err = ferror(fp);
  fclose(fp);
  
  return err ? -1 : 0;
}