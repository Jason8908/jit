#ifndef JIT_HEX_H
#define JIT_HEX_H

#include <stddef.h>

/**
 * Encode `len` bytes of `data` as lowercase hex.
 *
 * Writes `2*len` characters to `out`, plus a null terminator. Thus,
 * `out` must be at least `2*len + 1` bytes long.
 */
void hex_encode(char *out, const unsigned char *data, size_t len);

/**
 * Decode `hex_len` hex characters into bytes.
 *
 * `hex_len` must be even. Accepts 0-9, a-f, A-F.
 * Writes `hex_len/2` bytes to `out`.
 *
 * Returns 0 on success, -1 on odd length or a non-hex character.
 * On failure `out` is unspecified.
 */
int hex_decode(unsigned char *out, const char *hex, size_t hex_len);

#endif