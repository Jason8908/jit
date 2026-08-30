#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jit/parse_options.h"
#include "jit/usage.h"

#define OPT_HELP_COLUMN 24
#define OPT_FORM_MAX 64

// usage output

static void fprint_options_usage(FILE *stream, const option_t *opts);
static const char *value_placeholder(const option_t *opt);
static void format_option(char *buf, size_t size, const option_t *opt);

_Noreturn void usage_with_options(const char *const usage[], const option_t *opts) {
  fprint_usage(stderr, usage);
  fprint_options_usage(stderr, opts);

  exit(JIT_EXIT_USAGE);
}

static void fprint_options_usage(FILE *stream, const option_t *opts) {
  int listed = 0;
  for (const option_t *opt = opts; opt != NULL && opt->type != OPTION_END; opt++) {
    char form[OPT_FORM_MAX];

    if (opt->short_name == 0 && opt->long_name == NULL) continue;

    if (!listed) {
      fprintf(stream, "\n");
      listed = 1;
    }

    format_option(form, sizeof form, opt);

    if (opt->help != NULL)
      fprintf(stream, "    %-*s %s\n", OPT_HELP_COLUMN, form, opt->help);
    else
      fprintf(stream, "    %s\n", form);
  }
}

static void format_option(char *buf, size_t size, const option_t *opt) {
  char value[OPT_FORM_MAX] = "";

  if (opt->type == OPTION_STR || opt->type == OPTION_INT)
    snprintf(value, sizeof value, " %s", value_placeholder(opt));

  if (opt->short_name != 0 && opt->long_name != NULL)
    snprintf(buf, size, "-%c, --%s%s", opt->short_name, opt->long_name, value);
  else if (opt->short_name != 0)
    snprintf(buf, size, "-%c%s", opt->short_name, value);
  else
    snprintf(buf, size, "    --%s%s", opt->long_name, value);
}

static const char *value_placeholder(const option_t *opt) {
  if (opt->argh != NULL) return opt->argh;

  return opt->type == OPTION_INT ? "<n>" : "<value>";
}

// parsing

static const option_t *find_long(const option_t *options, const char *name, size_t len);
static const option_t *find_short(const option_t *options, char name);
static int parse_long_option(const char *arg, const char **argv, int argc, int i, const option_t *options, const char *const usage[]);
static int parse_short_option(const char *arg, const char **argv, int argc, int i, const option_t *options, const char *const usage[]);
static void set_value(const option_t *opt, const char *value, const option_t *options, const char *const usage[]);

int parse_options(int argc, const char **argv, const option_t *options, const char *const usage[]) {
  int i = 1;

  while (i < argc) {
    const char *arg = argv[i];

    // We know when operands start by checking for the absence of a leading "-"
    // and also checking for "--" to mark the end of options
    if (arg[0] != '-' || arg[1] == '\0') break;
    if (strcmp(arg, "--") == 0) {
      i++;
      break;
    }

    i++;

    if (arg[1] == '-')
      i = parse_long_option(arg + 2, argv, argc, i, options, usage);
    else
      i = parse_short_option(arg + 1, argv, argc, i, options, usage);
  }

  int num_operands = argc - i;

  for (int j = 0; j < num_operands; j++)
    argv[j] = argv[i + j];

  return num_operands;
}

/**
 * Parse one "--name", "--name=value", "--name value" or "--no-name" argument, where `arg`
 * points past the leading "--". `i` is the index of the next unread argument;
 * the new index is returned.
 */
static int parse_long_option(const char *arg, const char **argv, int argc, int i, const option_t *options, const char *const usage[]) {
  const char *eq = strchr(arg, '=');
  size_t arg_len = eq != NULL ? (size_t)(eq - arg) : strlen(arg);
  const char *value = eq != NULL ? eq + 1 : NULL;

  // Prioritize the name as written first, so an option really called "no-foo" wins
  // over negating an option called "foo".
  const option_t *opt = find_long(options, arg, arg_len);
  int negated = 0;

  if (opt == NULL && arg_len > 3 && strncmp(arg, "no-", 3) == 0) {
    opt = find_long(options, arg + 3, arg_len - 3);
    negated = 1;
  }

  if (opt == NULL) {
    if (arg_len == 4 && strncmp(arg, "help", 4) == 0) usage_with_options(usage, options);

    error("unknown option: --%.*s", (int)arg_len, arg);
    usage_with_options(usage, options);
  }

  if (opt->type == OPTION_BOOL) {
    if (value != NULL) {
      error("option '--%s' takes no value", opt->long_name);
      usage_with_options(usage, options);
    }

    *(int *)opt->value = negated ? 0 : 1;
    return i;
  }

  if (negated) {
    error("option '--%s' cannot be negated", opt->long_name);
    usage_with_options(usage, options);
  }

  if (value == NULL) {
    if (i >= argc) {
      error("option '--%s' requires a value", opt->long_name);
      usage_with_options(usage, options);
    }

    value = argv[i++];
  }

  set_value(opt, value, options, usage);

  return i;
}

/**
 * Parse one short-option argument, where `arg` points past the leading "-".
 * Booleans cluster ("-wq"); the first value-taking option consumes the rest of
 * the cluster as its value, or the next argument if the cluster is exhausted.
 */
static int parse_short_option(const char *arg, const char **argv, int argc, int i, const option_t *options, const char *const usage[]) {
  while (*arg != '\0') {
    const option_t *opt = find_short(options, *arg);

    if (opt == NULL) {
      if (*arg == 'h') usage_with_options(usage, options);

      error("unknown option: -%c", *arg);
      usage_with_options(usage, options);
    }

    arg++;

    if (opt->type == OPTION_BOOL) {
      *(int *)opt->value = 1;
      continue;
    }

    const char *value = arg;

    if (*value == '\0') {
      if (i >= argc) {
        error("option '-%c' requires a value", opt->short_name);
        usage_with_options(usage, options);
      }

      value = argv[i++];
    }

    set_value(opt, value, options, usage);

    return i;
  }

  return i;
}

static const option_t *find_long(const option_t *options, const char *name, size_t len) {
  for (const option_t *opt = options; opt->type != OPTION_END; opt++) {
    if (opt->long_name != NULL && strlen(opt->long_name) == len &&
        strncmp(opt->long_name, name, len) == 0)
      return opt;
  }

  return NULL;
}

static const option_t *find_short(const option_t *options, char name) {
  for (const option_t *opt = options; opt->type != OPTION_END; opt++) {
    if (opt->short_name == name) return opt;
  }

  return NULL;
}

/**
 * Store `value` through `opt`, converting it first for OPTION_INT.
 * Does not return if the value is not a well-formed int.
 */
static void set_value(const option_t *opt, const char *value, const option_t *options, const char *const usage[]) {
  if (opt->type == OPTION_STR) {
    *(const char **)opt->value = value;
    return;
  }

  char *end;
  errno = 0;
  long n = strtol(value, &end, 10);

  if (*value == '\0' || *end != '\0' || errno == ERANGE || n < INT_MIN ||
      n > INT_MAX) {
    error("option '--%s' expects a number, got '%s'", opt->long_name, value);
    usage_with_options(usage, options);
  }

  *(int *)opt->value = (int)n;
}