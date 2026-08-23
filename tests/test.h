#ifndef JIT_TEST_H
#define JIT_TEST_H

#include <stdio.h>

/**
 * A minimal test harness.
 *
 * Each module gets its own test binary with its own main(), so the
 * counters below are file-static and never shared between suites.
 *
 * A failing ASSERT returns from the test immediately, so any
 * cleanup at the bottom of that test is skipped.
 */

static int test_failures;
static int test_current_failed;

#define ASSERT(cond)                                                          \
  do {                                                                        \
    if (!(cond)) {                                                            \
      fprintf(stderr, "    %s:%d: assertion failed: %s\n", __FILE__,          \
              __LINE__, #cond);                                               \
      test_current_failed = 1;                                                \
      return;                                                                 \
    }                                                                         \
  } while (0)

#define RUN_TEST(fn)                                                          \
  do {                                                                        \
    test_current_failed = 0;                                                  \
    fn();                                                                     \
    if (test_current_failed) {                                                \
      test_failures++;                                                        \
      printf("  FAIL %s\n", #fn);                                             \
    } else {                                                                  \
      printf("  ok   %s\n", #fn);                                             \
    }                                                                         \
  } while (0)

#define TEST_SUMMARY()                                                        \
  (printf("%s: %d failure(s)\n", __FILE__, test_failures),                    \
   test_failures ? 1 : 0)

#endif
