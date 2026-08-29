#!/bin/sh
# End-to-end tests for the top-level dispatcher in src/cli.c.

. "$(dirname "$0")/harness.sh"

test_begin 'bare jit prints usage to stderr and exits 1'
  run
  assert_status 1
  assert_stderr_contains 'usage: jit'
  assert_stdout_empty
test_end

test_begin 'jit help prints the command list to stdout and exits 0'
  run help
  assert_status 0
  assert_stdout_contains 'These are the jit commands'
  assert_stderr_empty
test_end

test_begin 'jit --help matches jit help'
  run --help
  assert_status 0
  assert_stdout_contains 'These are the jit commands'
  assert_stderr_empty
test_end

test_begin 'jit --version prints the version to stdout'
  run --version
  assert_status 0
  assert_stdout_contains 'jit version'
  assert_stderr_empty
test_end

test_begin 'help output lists every registered command'
  run help
  assert_stdout_contains 'help'
  assert_stdout_contains 'hash-object'
test_end

test_begin 'unknown command exits 1 with a message on stderr'
  run frobnicate
  assert_status 1
  assert_stderr_contains "'frobnicate' is not a jit command"
  assert_stdout_empty
test_end

test_begin 'unknown global option exits 129'
  run --bogus
  assert_status 129
  assert_stderr_contains 'unknown option: --bogus'
  assert_stdout_empty
test_end

test_begin 'empty-string command falls through to lookup and exits 1'
  run ''
  assert_status 1
  assert_stderr_contains 'is not a jit command'
  assert_stdout_empty
test_end

test_begin 'bare dash is treated as a global option and exits 129'
  run -
  assert_status 129
  assert_stderr_contains 'unknown option: -'
  assert_stdout_empty
test_end

test_summary
