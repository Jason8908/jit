#include <stdio.h>
#include "jit/strbuf.h"

int main() {
  strbuf_t buf;
  strbuf_init(&buf, 1);

  strbuf_cat(&buf, "Hello, world!", 13);
  fwrite(buf.buf, 1, buf.len, stdout);
  printf("\n");

  strbuf_release(&buf);
}