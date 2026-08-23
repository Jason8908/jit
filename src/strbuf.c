#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "jit/strbuf.h"

static const size_t STRBUF_MIN_ALLOC = 16;

/**
 * Initialize a strbuf_t with an initial size.
 *
 * *sb must be a valid pointer to an uninitialized strbuf_t.
 * initial_size must be a valid size_t value.
 */
void strbuf_init(strbuf_t *sb, size_t initial_size) {
  sb->alloc = sb->len = 0;
  sb->buf = NULL;

  if (initial_size > 0)
    strbuf_grow(sb, initial_size);
}

/**
 * Release the memory associated with a strbuf_t.
 *
 * *sb must be a valid pointer to an already initialized strbuf_t.
 */
void strbuf_release(strbuf_t *sb) {
  free(sb->buf);
  sb->buf = NULL;
  sb->alloc = sb->len = 0;
}

/**
 * Append data to a strbuf_t.
 * If there is not enough remaining free space, the buffer will
 * be resized to accommodate the new data.
 *
 * *sb must be a valid pointer to an already initialized strbuf_t.
 * data must be a valid pointer to a block of data to append.
 * len must be a valid size_t value.
 */
void strbuf_cat(strbuf_t *sb, const void *data, size_t len) {
  strbuf_grow(sb, len);
  memcpy(sb->buf + sb->len, data, len);
  sb->len += len;
}

/**
 * Append a formatted string to a strbuf_t.
 * If there is not enough remaining free space, the buffer will
 * be resized to accommodate the new data.
 *
 * *sb must be a valid pointer to an already initialized strbuf_t.
 * fstr must be a valid pointer to a formatted string.
 * ... must be a valid variadic argument list.
 */
void strbuf_catf(strbuf_t *sb, const char *fstr, ...) {
  va_list ap;
  va_start(ap, fstr);

  va_list aq;
  va_copy(aq, ap);
  
  int n = vsnprintf(NULL, 0, fstr, aq);
  va_end(aq);

  if (n < 0) abort();
  if ((size_t)n > SIZE_MAX - 1) abort();

  strbuf_grow(sb, (size_t)n + 1);

  vsnprintf(sb->buf + sb->len, (size_t)n + 1, fstr, ap);
  va_end(ap);

  sb->len += (size_t)n;
}

/**
 * Grow the buffer so that there is at least len bytes
 * of free space.
 * 
 * This will be a no-op if the buffer already has enough free space.
 *
 * *sb must be a valid pointer to an already initialized strbuf_t.
 * len must be a valid size_t value.
 */
void strbuf_grow(strbuf_t *sb, size_t len) {
  if (sb->len <= sb->alloc && sb->alloc - sb->len >= len) return;
  if (len > SIZE_MAX - sb->len) abort();

  size_t min_alloc = sb->len + len;

  size_t new_alloc;
  if (sb->alloc > SIZE_MAX / 2)
    new_alloc = min_alloc;
  else
    new_alloc = sb->alloc ? sb->alloc * 2 : STRBUF_MIN_ALLOC;

  if (new_alloc < min_alloc)
    new_alloc = min_alloc;

  char *p = realloc(sb->buf, new_alloc);
  if (!p) abort();

  sb->buf = p;
  sb->alloc = new_alloc;
}