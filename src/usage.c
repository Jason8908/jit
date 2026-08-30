#include "jit/usage.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

_Noreturn void usage(const char *const *lines) {
  fprint_usage(stderr, lines);

  exit(JIT_EXIT_USAGE);
}

void fprint_usage(FILE *stream, const char *const *lines) {
  if (lines == NULL || lines[0] == NULL) return;

  fprintf(stream, "usage: %s\n", lines[0]);

  for (size_t i = 1; lines[i] != NULL; i++) {
    fprintf(stream, "   or: %s\n", lines[i]);
  }
}

_Noreturn void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  fprintf(stderr, "fatal: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  va_end(ap);

  exit(JIT_EXIT_FATAL);
}
 
int error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  va_end(ap);

  return JIT_ERROR_CODE_ERROR;
}
  
void warning(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  fprintf(stderr, "warning: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  va_end(ap);
}