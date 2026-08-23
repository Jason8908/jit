#include <stdio.h>
#include <string.h>
#include "jit/strbuf.h"
#include "jit/blob.h"
#include "jit/object.h"


int main() {
  const char *path = "./hello.txt";
  strbuf_t payload;
  oid_sha1_t oid;
  char oid_str[41];

  strbuf_init(&payload, 0);
  if (blob_read_path(&payload, path) != 0) {
    fprintf(stderr, "Failed to read blob from %s\n", path);
    return 1;
  }

  object_hash(&oid, OBJ_BLOB, payload.buf, payload.len);
  oid_sha1_fmt(oid_str, &oid);
  printf("%s\n", oid_str);

  strbuf_release(&payload);
}