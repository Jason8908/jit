#ifndef JIT_STRBUF_H
#define JIT_STRBUF_H

#include <stddef.h>

/**
 * A simple growing byte buffer for storing primarily
 * byte data.
 *
 * Does not automatically include extra room for a \0 terminator.
 */
typedef struct strbuf {
  char *buf;
  size_t len;
  size_t alloc;
} strbuf_t;

void strbuf_init(strbuf_t *sb, size_t initial_size);
void strbuf_release(strbuf_t *sb);
void strbuf_cat(strbuf_t *sb, const void *data, size_t len);
void strbuf_catf(strbuf_t *sb, const char *fstr, ...);
void strbuf_grow(strbuf_t *sb, size_t len);

#endif