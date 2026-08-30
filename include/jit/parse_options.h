#ifndef JIT_PARSE_OPTIONS_H
#define JIT_PARSE_OPTIONS_H

typedef enum {
  OPTION_END,
  OPTION_BOOL,
  OPTION_STR,
  OPTION_INT
} opt_type_t;

typedef struct option {
  opt_type_t type;
  int short_name;
  const char *long_name;
  void *value;
  const char *argh;
  const char *help;
} option_t;

#define OPT_BOOL(s, l, v, h)       { OPTION_BOOL,   (s), (l), (v), NULL, (h) }
#define OPT_STRING(s, l, v, a, h)  { OPTION_STR,    (s), (l), (v), (a),  (h) }
#define OPT_INTEGER(s, l, v, h)    { OPTION_INT,    (s), (l), (v), NULL, (h) }
#define OPT_END()                  { OPTION_END, 0, NULL, NULL, NULL, NULL }

/**
 * Parse command-line options in `argv` according to the `options` table.
 *
 * `argv[0]` is the subcommand's own name and will be skipped.
 *
 * Recognized forms:
 *
 *   --name              boolean
 *   --no-name           boolean, cleared
 *   --name=value        value attached
 *   --name value        value as the next argument
 *   -n                  boolean
 *   -abc                clustered booleans
 *   -nvalue             value attached
 *   -n value            value as the next argument
 *   --                  ends option parsing; everything after is an operand
 */
int parse_options(int argc, const char **argv, const option_t *options, const char *const usage[]);

/**
 * Print a usage message built from `usage` and `options` to stderr, then
 * exit(JIT_EXIT_USAGE).
 */
_Noreturn void usage_with_options(const char *const usage[], const option_t *opts);

#endif