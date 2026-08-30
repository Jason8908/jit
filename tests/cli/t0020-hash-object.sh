#!/bin/sh
# End-to-end tests for `jit hash-object` (src/cmd_hash_object.c).

. "$(dirname "$0")/harness.sh"

OID_EMPTY='e69de29bb2d1d6434b8b29ae775ad8c2e48c5391'
OID_HELLO_WORLD='95d09f2b10159347eece71399a7e2e907ea3df4f'
OID_HELLO_NL='ce013625030ba8dba906f756967f9e9ca394464a'
OID_4096_A='9d235ed07cd19811a6ceb342de82f190e49c9f68'
OID_10000_B='8722004706ee9e9e08ec3f12d9181a10182bff42'
OID_NUL_BYTES='9583496fd9b881325fc7085e7d6b84ca0573355d'

nth_line() { sed -n "$1p" "$OUT"; }

# correctness

test_begin 'hashes an empty file'
  : > empty.txt
  run hash-object empty.txt
  assert_status 0
  assert_stdout_equals "$OID_EMPTY"
  assert_stderr_empty
test_end

test_begin 'hashes a file with no trailing newline'
  printf '%s' 'hello world' > hw.txt
  run hash-object hw.txt
  assert_status 0
  assert_stdout_equals "$OID_HELLO_WORLD"
test_end

test_begin 'hashes a file with a trailing newline'
  printf 'hello\n' > hn.txt
  run hash-object hn.txt
  assert_status 0
  assert_stdout_equals "$OID_HELLO_NL"
test_end

test_begin 'hashes a file of exactly CHUNK_SIZE (4096) bytes'
  head -c 4096 /dev/zero | tr '\0' 'a' > k4.txt
  run hash-object k4.txt
  assert_status 0
  assert_stdout_equals "$OID_4096_A"
test_end

test_begin 'hashes a file spanning multiple read chunks (10000 bytes)'
  head -c 10000 /dev/zero | tr '\0' 'b' > k10.txt
  run hash-object k10.txt
  assert_status 0
  assert_stdout_equals "$OID_10000_B"
test_end

test_begin 'hashes binary content containing NUL bytes'
  { printf 'a'; head -c 1 /dev/zero; printf 'b'; head -c 1 /dev/zero; printf 'c'; } > bin.txt
  assert_equals 5 "$(wc -c < bin.txt | tr -d ' ')" 'fixture size'
  run hash-object bin.txt
  assert_status 0
  assert_stdout_equals "$OID_NUL_BYTES"
test_end

test_begin 'identical content under different names hashes identically'
  printf '%s' 'same' > one.txt
  printf '%s' 'same' > two.txt
  run hash-object one.txt two.txt
  assert_status 0
  assert_stdout_lines 2
  assert_equals "$(nth_line 1)" "$(nth_line 2)" 'oids'
test_end

test_begin 'relative and absolute paths hash identically'
  run hash-object hw.txt
  rel="$(nth_line 1)"
  run hash-object "$PWD/hw.txt"
  assert_status 0
  assert_equals "$rel" "$(nth_line 1)" 'oids'
test_end

# multi-file

test_begin 'hashes several files in argument order'
  run hash-object empty.txt hw.txt hn.txt
  assert_status 0
  assert_stdout_lines 3
  assert_equals "$OID_EMPTY" "$(nth_line 1)" 'line 1'
  assert_equals "$OID_HELLO_WORLD" "$(nth_line 2)" 'line 2'
  assert_equals "$OID_HELLO_NL" "$(nth_line 3)" 'line 3'
test_end

test_begin 'accepts the same file twice and prints it twice'
  run hash-object hw.txt hw.txt
  assert_status 0
  assert_stdout_lines 2
  assert_equals "$OID_HELLO_WORLD" "$(nth_line 1)" 'line 1'
  assert_equals "$OID_HELLO_WORLD" "$(nth_line 2)" 'line 2'
test_end

# usage and errors

test_begin 'no operands prints usage to stderr and exits 129'
  run hash-object
  assert_status 129
  assert_stderr_contains 'usage: jit hash-object'
  assert_stdout_empty
test_end

test_begin 'nonexistent file exits 128 naming the path and the reason'
  run hash-object missing.txt
  assert_status 128
  assert_stderr_contains 'missing.txt'
  assert_stderr_contains 'No such file or directory'
  assert_stdout_empty
test_end

test_begin 'directory operand exits 128'
  mkdir -p adir
  run hash-object adir
  assert_status 128
  assert_stderr_contains 'Is a directory'
  assert_stdout_empty
test_end

if [ "$(id -u)" = "0" ]; then
  skip 'unreadable file exits 128' 'running as root ignores mode bits'
else
  test_begin 'unreadable file exits 128'
    printf '%s' 'secret' > locked.txt
    chmod 000 locked.txt
    run hash-object locked.txt
    assert_status 128
    assert_stderr_contains 'Permission denied'
    assert_stdout_empty
    chmod 644 locked.txt
  test_end
fi

test_begin 'empty-string operand exits 128'
  run hash-object ''
  assert_status 128
  assert_stdout_empty
test_end

test_begin 'a filename containing spaces is handled'
  printf '%s' 'hello world' > 'sp ace.txt'
  run hash-object 'sp ace.txt'
  assert_status 0
  assert_stdout_equals "$OID_HELLO_WORLD"
test_end

test_begin 'dash is treated as a filename, not stdin'
  run hash-object -
  assert_status 128
  assert_stdout_empty
test_end

#  partial output

test_begin 'a later unreadable file still emits earlier oids, then exits 128'
  run hash-object hw.txt missing.txt
  assert_status 128
  assert_stdout_lines 1
  assert_stdout_equals "$OID_HELLO_WORLD"
  assert_stderr_contains 'missing.txt'
test_end

test_begin 'an unreadable first file emits nothing'
  run hash-object missing.txt hw.txt
  assert_status 128
  assert_stdout_empty
test_end

# ---------------------------------------------------------------------- stdin

test_begin '--stdin hashes content read from standard input'
  printf '%s' 'hello world' > in.txt
  run_stdin in.txt hash-object --stdin
  assert_status 0
  assert_stdout_equals "$OID_HELLO_WORLD"
  assert_stderr_empty
test_end

test_begin '--stdin hashes empty input'
  : > in_empty.txt
  run_stdin in_empty.txt hash-object --stdin
  assert_status 0
  assert_stdout_equals "$OID_EMPTY"
test_end

test_begin '--stdin hashes input spanning multiple read chunks'
  head -c 10000 /dev/zero | tr '\0' 'b' > in_big.txt
  run_stdin in_big.txt hash-object --stdin
  assert_status 0
  assert_stdout_equals "$OID_10000_B"
test_end

test_begin '--stdin hashes binary input containing NUL bytes'
  { printf 'a'; head -c 1 /dev/zero; printf 'b'; head -c 1 /dev/zero; printf 'c'; } > in_bin.txt
  run_stdin in_bin.txt hash-object --stdin
  assert_status 0
  assert_stdout_equals "$OID_NUL_BYTES"
test_end

test_begin '--stdin and a file operand agree on the same content'
  run hash-object hw.txt
  from_file="$(nth_line 1)"
  run_stdin hw.txt hash-object --stdin
  assert_status 0
  assert_equals "$from_file" "$(nth_line 1)" 'oids'
test_end

test_begin '--stdin rejects file operands'
  run_stdin in.txt hash-object --stdin hw.txt
  assert_status 129
  assert_stderr_contains 'takes no file arguments'
  assert_stdout_empty
test_end

test_begin '-h prints usage and exits 129'
  run hash-object -h
  assert_status 129
  assert_stderr_contains 'usage: jit hash-object'
  assert_stderr_contains '--stdin'
  assert_stdout_empty
test_end

# parse_options stops at the first operand, so a flag after one is a filename.
test_begin '--stdin after an operand is treated as a filename'
  run hash-object hw.txt --stdin
  assert_status 128
  assert_stdout_lines 1
  assert_stdout_equals "$OID_HELLO_WORLD"
  assert_stderr_contains 'stdin'
test_end

test_summary
