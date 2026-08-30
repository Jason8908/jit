#ifndef JIT_USAGE_H
#define JIT_USAGE_H

#define JIT_ERROR_CODE_ERROR -1
#define JIT_EXIT_FATAL 128
#define JIT_EXIT_USAGE 129

#if defined(__GNUC__)
#define JIT_PRINTF_FMT(fmt_idx, args_idx) \
  __attribute__((format(printf, fmt_idx, args_idx)))
#else
#define JIT_PRINTF_FMT(fmt_idx, args_idx)
#endif

#include <stdio.h>

/**
 * Print each line of `lines` to stderr as a usage message, then exit(129).
 *
 * `lines` is a NULL-terminated array of usage strings.
 * The lines will be prefixed with "usage: " and "   or: " respectively.
 */
_Noreturn void usage(const char *const *lines);

/**
 * Print each line of `lines` to `stream` as a usage message.
 *
 * `lines` is a NULL-terminated array of usage strings.
 * The lines will be prefixed with "usage: " and "   or: " respectively.
 */
void fprint_usage(FILE *stream, const char *const *lines);

/**
 * Report a fatal error to stderr as "fatal: <message>" and exit(128).
 *
 * For unrecoverable failures only. Must not be called from the object
 * layer (object.c, blob.c, hash.c, hex.c, strbuf.c), which reports
 * failure by return value.
 */
 _Noreturn void die(const char *fmt, ...) JIT_PRINTF_FMT(1, 2);

 /**
  * Report an error to stderr as "error: <message>".
  *
  * Always returns -1, so callers can write `return error(...)`.
  */
 int error(const char *fmt, ...) JIT_PRINTF_FMT(1, 2);
 
 /**
  * Report a non-fatal condition to stderr as "warning: <message>".
  */
 void warning(const char *fmt, ...) JIT_PRINTF_FMT(1, 2);

#endif