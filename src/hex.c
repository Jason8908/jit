#include "jit/hex.h"

static const char hex_digits[] = "0123456789abcdef";

/**
 * Convert a hex character to a byte.
 * Returns the byte value, or -1 if the character is not a valid hex character.
 */
static int hex_to_byte(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

void hex_encode(char *out, const unsigned char *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    out[i * 2] = hex_digits[(data[i] >> 4)];
    out[i * 2 + 1] = hex_digits[data[i] & 0x0f];
  }
  out[len * 2] = '\0';
}

int hex_decode(unsigned char *out, const char *hex, size_t hex_len) {
  if (hex_len % 2 != 0) return -1;

  for (size_t i = 0; i < hex_len; i += 2) {
    int hi = hex_to_byte(hex[i]);
    int lo = hex_to_byte(hex[i + 1]);
    if (hi == -1 || lo == -1) return -1;

    out[i / 2] = (unsigned char)((hi << 4) | lo);
  }

  return 0;
}