#ifndef JIT_BUILTIN_H
#define JIT_BUILTIN_H

/**
 * Implementation of the 'hash-object' command.
 */
int cmd_hash_object(int argc, const char **argv);

/**
 * Implementation of the 'init' command.
 */
int cmd_init(int argc, const char **argv);

#endif