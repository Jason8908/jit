#include <string.h>
#include "jit/parse_options.h"
#include "test.h"

/**
 * Tests for the option parser.
 */

static const char *const test_usage[] = { "test [<options>] [<args>]", NULL };

#define ARGC(a) ((int)(sizeof(a) / sizeof((a)[0])))

// operands and `--`

static void test_no_arguments_returns_zero(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(flag == 0);
}

static void test_operands_only_are_returned_in_order(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "a.txt", "b.txt", "c.txt" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 3);
  ASSERT(strcmp(argv[0], "a.txt") == 0);
  ASSERT(strcmp(argv[1], "b.txt") == 0);
  ASSERT(strcmp(argv[2], "c.txt") == 0);
}

static void test_defaults_are_untouched_when_no_options_given(void) {
  int flag = 1;
  const char *type = "blob";
  int depth = 7;
  const option_t opts[] = {
    OPT_BOOL('v', "verbose", &flag, "verbose"),
    OPT_STRING('t', "type", &type, "<type>", "type"),
    OPT_INTEGER('d', "depth", &depth, "depth"),
    OPT_END()
  };
  const char *argv[] = { "cmd", "a.txt" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 1);
  ASSERT(flag == 1);
  ASSERT(strcmp(type, "blob") == 0);
  ASSERT(depth == 7);
}

static void test_parsing_stops_at_first_operand(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "a.txt", "--verbose" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 2);
  ASSERT(strcmp(argv[0], "a.txt") == 0);
  ASSERT(strcmp(argv[1], "--verbose") == 0);
  ASSERT(flag == 0);
}

static void test_double_dash_ends_option_parsing(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--verbose", "--", "a.txt" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 1);
  ASSERT(strcmp(argv[0], "a.txt") == 0);
  ASSERT(flag == 1);
}

static void test_double_dash_protects_option_like_operands(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--", "--verbose", "-v" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 2);
  ASSERT(strcmp(argv[0], "--verbose") == 0);
  ASSERT(strcmp(argv[1], "-v") == 0);
  ASSERT(flag == 0);
}

static void test_double_dash_alone_yields_no_operands(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
}

static void test_lone_dash_is_an_operand(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "-" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 1);
  ASSERT(strcmp(argv[0], "-") == 0);
}

static void test_empty_string_is_an_operand(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL('v', "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 1);
  ASSERT(strcmp(argv[0], "") == 0);
}

// long bools

static void test_long_bool_sets_flag(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL(0, "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--verbose" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(flag == 1);
}

static void test_multiple_long_bools(void) {
  int a = 0, b = 0, c = 0;
  const option_t opts[] = {
    OPT_BOOL(0, "alpha", &a, "a"),
    OPT_BOOL(0, "beta", &b, "b"),
    OPT_BOOL(0, "gamma", &c, "c"),
    OPT_END()
  };
  const char *argv[] = { "cmd", "--gamma", "--alpha" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(a == 1);
  ASSERT(b == 0);
  ASSERT(c == 1);
}

static void test_repeated_long_bool_stays_set(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL(0, "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--verbose", "--verbose" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(flag == 1);
}

// long values

static void test_long_value_attached_with_equals(void) {
  const char *type = "blob";
  const option_t opts[] = { OPT_STRING(0, "type", &type, "<type>", "type"), OPT_END() };
  const char *argv[] = { "cmd", "--type=tree" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(strcmp(type, "tree") == 0);
}

static void test_long_value_as_separate_argument(void) {
  const char *type = "blob";
  const option_t opts[] = { OPT_STRING(0, "type", &type, "<type>", "type"), OPT_END() };
  const char *argv[] = { "cmd", "--type", "tree" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(strcmp(type, "tree") == 0);
}

static void test_long_value_can_be_empty(void) {
  const char *type = "blob";
  const option_t opts[] = { OPT_STRING(0, "type", &type, "<type>", "type"), OPT_END() };
  const char *argv[] = { "cmd", "--type=" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(strcmp(type, "") == 0);
}

/**
 * The separate form consumes the next argument verbatim, even when it looks
 * like another option. Deliberate v1 simplification: it keeps the parser from
 * needing to know whether "--verbose" was meant as a value or a flag.
 */
static void test_long_value_consumes_next_argument_verbatim(void) {
  const char *type = "blob";
  int flag = 0;
  const option_t opts[] = {
    OPT_STRING(0, "type", &type, "<type>", "type"),
    OPT_BOOL(0, "verbose", &flag, "verbose"),
    OPT_END()
  };
  const char *argv[] = { "cmd", "--type", "--verbose" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(strcmp(type, "--verbose") == 0);
  ASSERT(flag == 0);
}

static void test_last_long_value_wins(void) {
  const char *type = "blob";
  const option_t opts[] = { OPT_STRING(0, "type", &type, "<type>", "type"), OPT_END() };
  const char *argv[] = { "cmd", "--type=tree", "--type", "commit" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(strcmp(type, "commit") == 0);
}

static void test_long_value_followed_by_operands(void) {
  const char *type = "blob";
  const option_t opts[] = { OPT_STRING(0, "type", &type, "<type>", "type"), OPT_END() };
  const char *argv[] = { "cmd", "--type", "tree", "a.txt", "b.txt" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 2);
  ASSERT(strcmp(type, "tree") == 0);
  ASSERT(strcmp(argv[0], "a.txt") == 0);
  ASSERT(strcmp(argv[1], "b.txt") == 0);
}

// short bools

static void test_short_bool_sets_flag(void) {
  int write_obj = 0;
  const option_t opts[] = { OPT_BOOL('w', NULL, &write_obj, "write"), OPT_END() };
  const char *argv[] = { "cmd", "-w" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(write_obj == 1);
}

static void test_separate_short_bools(void) {
  int w = 0, q = 0;
  const option_t opts[] = {
    OPT_BOOL('w', NULL, &w, "write"),
    OPT_BOOL('q', NULL, &q, "quiet"),
    OPT_END()
  };
  const char *argv[] = { "cmd", "-w", "-q" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(w == 1);
  ASSERT(q == 1);
}

// short values

static void test_short_value_as_separate_argument(void) {
  const char *type = "blob";
  const option_t opts[] = { OPT_STRING('t', "type", &type, "<type>", "type"), OPT_END() };
  const char *argv[] = { "cmd", "-t", "tree" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(strcmp(type, "tree") == 0);
}

static void test_short_value_attached(void) {
  const char *type = "blob";
  const option_t opts[] = { OPT_STRING('t', "type", &type, "<type>", "type"), OPT_END() };
  const char *argv[] = { "cmd", "-ttree" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(strcmp(type, "tree") == 0);
}

// clustering

static void test_clustered_short_bools(void) {
  int w = 0, q = 0, v = 0;
  const option_t opts[] = {
    OPT_BOOL('w', NULL, &w, "write"),
    OPT_BOOL('q', NULL, &q, "quiet"),
    OPT_BOOL('v', NULL, &v, "verbose"),
    OPT_END()
  };
  const char *argv[] = { "cmd", "-wqv" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(w == 1);
  ASSERT(q == 1);
  ASSERT(v == 1);
}

static void test_cluster_ending_in_value_option_takes_next_argument(void) {
  int w = 0;
  const char *type = "blob";
  const option_t opts[] = {
    OPT_BOOL('w', NULL, &w, "write"),
    OPT_STRING('t', "type", &type, "<type>", "type"),
    OPT_END()
  };
  const char *argv[] = { "cmd", "-wt", "tree" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(w == 1);
  ASSERT(strcmp(type, "tree") == 0);
}

static void test_cluster_with_attached_value(void) {
  int w = 0;
  const char *type = "blob";
  const option_t opts[] = {
    OPT_BOOL('w', NULL, &w, "write"),
    OPT_STRING('t', "type", &type, "<type>", "type"),
    OPT_END()
  };
  const char *argv[] = { "cmd", "-wttree" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(w == 1);
  ASSERT(strcmp(type, "tree") == 0);
}

// negation

static void test_no_prefix_clears_bool(void) {
  int flag = 1;
  const option_t opts[] = { OPT_BOOL(0, "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--no-verbose" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(flag == 0);
}

static void test_no_prefix_overrides_earlier_set(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL(0, "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--verbose", "--no-verbose" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(flag == 0);
}

static void test_set_after_no_prefix_wins(void) {
  int flag = 0;
  const option_t opts[] = { OPT_BOOL(0, "verbose", &flag, "verbose"), OPT_END() };
  const char *argv[] = { "cmd", "--no-verbose", "--verbose" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(flag == 1);
}

// integers

static void test_integer_value_as_separate_argument(void) {
  int depth = 0;
  const option_t opts[] = { OPT_INTEGER('d', "depth", &depth, "depth"), OPT_END() };
  const char *argv[] = { "cmd", "--depth", "42" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(depth == 42);
}

static void test_integer_value_attached(void) {
  int depth = 0;
  const option_t opts[] = { OPT_INTEGER('d', "depth", &depth, "depth"), OPT_END() };
  const char *argv[] = { "cmd", "--depth=7" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(depth == 7);
}

static void test_integer_accepts_zero(void) {
  int depth = 99;
  const option_t opts[] = { OPT_INTEGER('d', "depth", &depth, "depth"), OPT_END() };
  const char *argv[] = { "cmd", "-d", "0" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(depth == 0);
}

static void test_integer_accepts_negative(void) {
  int depth = 0;
  const option_t opts[] = { OPT_INTEGER('d', "depth", &depth, "depth"), OPT_END() };
  const char *argv[] = { "cmd", "--depth=-3" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 0);
  ASSERT(depth == -3);
}

// integration

static void test_short_and_long_forms_set_the_same_variable(void) {
  const char *a = "x", *b = "x";
  const option_t opts_a[] = { OPT_STRING('t', "type", &a, "<type>", "type"), OPT_END() };
  const option_t opts_b[] = { OPT_STRING('t', "type", &b, "<type>", "type"), OPT_END() };
  const char *argv_a[] = { "cmd", "-t", "tree" };
  const char *argv_b[] = { "cmd", "--type=tree" };

  ASSERT(parse_options(ARGC(argv_a), argv_a, opts_a, test_usage) == 0);
  ASSERT(parse_options(ARGC(argv_b), argv_b, opts_b, test_usage) == 0);
  ASSERT(strcmp(a, "tree") == 0);
  ASSERT(strcmp(b, "tree") == 0);
}

static void test_realistic_command_line(void) {
  const char *type = "blob";
  int write_obj = 0, from_stdin = 0;
  const option_t opts[] = {
    OPT_STRING('t', "type", &type, "<type>", "object type"),
    OPT_BOOL('w', NULL, &write_obj, "write the object"),
    OPT_BOOL(0, "stdin", &from_stdin, "read from stdin"),
    OPT_END()
  };
  const char *argv[] = { "hash-object", "-w", "--type=tree", "a.txt", "b.txt" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 2);
  ASSERT(write_obj == 1);
  ASSERT(from_stdin == 0);
  ASSERT(strcmp(type, "tree") == 0);
  ASSERT(strcmp(argv[0], "a.txt") == 0);
  ASSERT(strcmp(argv[1], "b.txt") == 0);
}

static void test_operands_are_moved_to_the_front_of_argv(void) {
  int w = 0;
  const option_t opts[] = { OPT_BOOL('w', NULL, &w, "write"), OPT_END() };
  const char *argv[] = { "cmd", "-w", "only.txt" };

  ASSERT(parse_options(ARGC(argv), argv, opts, test_usage) == 1);
  ASSERT(strcmp(argv[0], "only.txt") == 0);
}

int main(void) {
  RUN_TEST(test_no_arguments_returns_zero);
  RUN_TEST(test_operands_only_are_returned_in_order);
  RUN_TEST(test_defaults_are_untouched_when_no_options_given);
  RUN_TEST(test_parsing_stops_at_first_operand);
  RUN_TEST(test_double_dash_ends_option_parsing);
  RUN_TEST(test_double_dash_protects_option_like_operands);
  RUN_TEST(test_double_dash_alone_yields_no_operands);
  RUN_TEST(test_lone_dash_is_an_operand);
  RUN_TEST(test_empty_string_is_an_operand);

  RUN_TEST(test_long_bool_sets_flag);
  RUN_TEST(test_multiple_long_bools);
  RUN_TEST(test_repeated_long_bool_stays_set);

  RUN_TEST(test_long_value_attached_with_equals);
  RUN_TEST(test_long_value_as_separate_argument);
  RUN_TEST(test_long_value_can_be_empty);
  RUN_TEST(test_long_value_consumes_next_argument_verbatim);
  RUN_TEST(test_last_long_value_wins);
  RUN_TEST(test_long_value_followed_by_operands);

  RUN_TEST(test_short_bool_sets_flag);
  RUN_TEST(test_separate_short_bools);

  RUN_TEST(test_short_value_as_separate_argument);
  RUN_TEST(test_short_value_attached);

  RUN_TEST(test_clustered_short_bools);
  RUN_TEST(test_cluster_ending_in_value_option_takes_next_argument);
  RUN_TEST(test_cluster_with_attached_value);

  RUN_TEST(test_no_prefix_clears_bool);
  RUN_TEST(test_no_prefix_overrides_earlier_set);
  RUN_TEST(test_set_after_no_prefix_wins);

  RUN_TEST(test_integer_value_as_separate_argument);
  RUN_TEST(test_integer_value_attached);
  RUN_TEST(test_integer_accepts_zero);
  RUN_TEST(test_integer_accepts_negative);

  RUN_TEST(test_short_and_long_forms_set_the_same_variable);
  RUN_TEST(test_realistic_command_line);
  RUN_TEST(test_operands_are_moved_to_the_front_of_argv);

  return TEST_SUMMARY();
}
