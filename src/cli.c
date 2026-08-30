#include <stdio.h>
#include <string.h>

#include "jit/cli.h"
#include "jit/shared.h"
#include "jit/usage.h"
#include "jit/builtin.h"

#define JIT_VERSION "0.1.1"

typedef int (*cmd_fn_t)(int argc, const char **argv);

typedef struct {
  const char *name;
  cmd_fn_t fn;
  const char *help;
} cmd_struct_t;

static const char *const jit_usage[] = {
  "jit <command> [<args>]",
  NULL
};

static int cmd_help(int argc, const char **argv);

static const cmd_struct_t commands[] = {
  { "help", cmd_help, "list available commands" },
  { "hash-object", cmd_hash_object, "compute and print the hash of an object" },
  { "init", cmd_init, "create an empty jit repository" },
};

static void print_version(void) {
  printf("jit version %s\n", JIT_VERSION);
}

static void print_command_list(FILE *out) {
  fprintf(out, "usage: %s\n", jit_usage[0]);
  for (size_t i = 1; jit_usage[i] != NULL; i++)
    fprintf(out, "   or: %s\n", jit_usage[i]);

  fprintf(out, "\nThese are the jit commands:\n\n");
  for (size_t i = 0; i < ARRAY_SIZE(commands); i++)
    fprintf(out, "   %-14s %s\n", commands[i].name, commands[i].help);
}

static const cmd_struct_t *lookup_command(const char *name) {
  for (size_t i = 0; i < ARRAY_SIZE(commands); i++) {
    if (strcmp(commands[i].name, name) == 0) return &commands[i];
  }

  return NULL;
}

static int cmd_help(int argc, const char **argv) {
  (void)argc;
  (void)argv;

  print_command_list(stdout);

  return 0;
}

int cli_run(int argc, char **argv) {
  argc--;
  argv++;

  // Parse options.
  while (argc > 0 && argv[0][0] == '-') {
    // Early returns on certain options.
    if (strcmp(argv[0], "--help") == 0) {
      print_command_list(stdout);
      return 0;
    }
    if (strcmp(argv[0], "--version") == 0) {
      print_version();
      return 0;
    }

    error("unknown option: %s", argv[0]);
    print_command_list(stderr);
    return JIT_EXIT_USAGE;
  }

  // No command provided after parsing options.
  if (argc == 0) {
    print_command_list(stderr);
    return 1;
  }

  // Lookup command and execute it if found.
  const cmd_struct_t *cmd = lookup_command(argv[0]);
  if (cmd == NULL) {
    error("'%s' is not a jit command. See 'jit help'.", argv[0]);
    return 1;
  }

  return cmd->fn(argc, (const char **)argv);
}
