# Shared assertion library for jit's end-to-end CLI tests.
#
# Environment:
#   JIT            path to the jit binary (default ./bin/jit)
#   JIT_TEST_KEEP  set to 1 to keep the trash directory after the run

set -u

: "${JIT:=./bin/jit}"

# Resolve JIT to an absolute path before cd'ing into the trash directory.
case "$JIT" in
  /*) ;;
  *)  JIT="$(cd "$(dirname "$JIT")" && pwd)/$(basename "$JIT")" ;;
esac

if [ ! -x "$JIT" ]; then
  echo "harness: '$JIT' is not executable" >&2
  exit 1
fi

SUITE="$(basename "$0")"

TRASH="$(mktemp -d)" || exit 1
cleanup() {
  if [ "${JIT_TEST_KEEP:-0}" = "1" ]; then
    echo "  (kept trash directory: $TRASH)"
  else
    rm -rf "$TRASH"
  fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

OUT="$TRASH/.stdout"
ERR="$TRASH/.stderr"
STATUS=0

suite_failures=0
case_failed=0
case_name=""

cd "$TRASH" || exit 1

# run <args...> -- invoke jit, capturing both streams and the exit status.
run() {
  "$JIT" "$@" >"$OUT" 2>"$ERR"
  STATUS=$?
  return 0
}

# run_stdin <file> <args...> -- like run(), with `file` on standard input.
run_stdin() {
  stdin_file="$1"
  shift
  "$JIT" "$@" <"$stdin_file" >"$OUT" 2>"$ERR"
  STATUS=$?
  return 0
}

fail() {
  if [ "$case_failed" -eq 0 ]; then
    printf '  FAIL %s\n' "$case_name"
    case_failed=1
  fi
  printf '    %s\n' "$1"
}

test_begin() {
  case_name="$1"
  case_failed=0
}

test_end() {
  if [ "$case_failed" -eq 0 ]; then
    printf '  ok   %s\n' "$case_name"
  else
    suite_failures=$((suite_failures + 1))
  fi
}

skip() {
  printf '  skip %s (%s)\n' "$1" "$2"
}

test_summary() {
  printf '%s: %d failure(s)\n' "$SUITE" "$suite_failures"
  [ "$suite_failures" -eq 0 ]
}

assert_status() {
  [ "$STATUS" -eq "$1" ] || fail "exit status: want $1, got $STATUS"
}

assert_stdout_equals() {
  got="$(cat "$OUT")"
  [ "$got" = "$1" ] || fail "stdout: want '$1', got '$got'"
}

assert_stdout_contains() {
  grep -qF -- "$1" "$OUT" || fail "stdout: missing '$1'"
}

assert_stdout_lines() {
  got="$(wc -l < "$OUT" | tr -d ' ')"
  [ "$got" -eq "$1" ] || fail "stdout: want $1 line(s), got $got"
}

assert_stdout_empty() {
  [ ! -s "$OUT" ] || fail "stdout: want empty, got '$(cat "$OUT")'"
}

assert_stderr_contains() {
  grep -qF -- "$1" "$ERR" || fail "stderr: missing '$1'"
}

assert_stderr_empty() {
  [ ! -s "$ERR" ] || fail "stderr: want empty, got '$(cat "$ERR")'"
}

# assert_equals <want> <got> <label>
assert_equals() {
  [ "$1" = "$2" ] || fail "$3: want '$1', got '$2'"
}
