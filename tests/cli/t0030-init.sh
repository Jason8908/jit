#!/bin/sh
# End-to-end tests for `jit init` (src/cmd_init.c).

. "$(dirname "$0")/harness.sh"

# fresh <name> -- move into an empty subdirectory of the trash directory.
fresh() {
  cd "$TRASH" || exit 1
  rm -rf "$1"
  mkdir "$1" || exit 1
  cd "$1" || exit 1
}

# Everything below the cwd.
tree_here() { find . | sort | tr '\n' ' '; }

jit_dir() { printf '%s/.jit' "$(pwd -P)"; }

# layout

test_begin 'creates .jit and .jit/objects and nothing else'
  fresh basic
  run init
  assert_status 0
  assert_dir_exists .jit
  assert_dir_exists .jit/objects
  assert_equals '. ./.jit ./.jit/objects ' "$(tree_here)" 'tree'
  assert_stderr_empty
test_end

test_begin 'reports the absolute path of the created .jit directory'
  fresh reports-path
  run init
  assert_status 0
  assert_stdout_equals "Initialized empty Jit repository in $(jit_dir)"
test_end

test_begin 'directory modes come from the umask, not a hardcoded value'
  fresh umask-honored
  old_umask="$(umask)"
  umask 077
  run init
  umask "$old_umask"
  assert_status 0
  assert_perms 'rwx------' .jit
  assert_perms 'rwx------' .jit/objects
test_end

# reinitialization

test_begin 'a second init succeeds, says so, and changes nothing'
  fresh second-init
  run init
  before="$(tree_here)"
  run init
  assert_status 0
  assert_stdout_equals "Reinitialized existing Jit repository in $(jit_dir)"
  assert_equals "$before" "$(tree_here)" 'tree'
  assert_stderr_empty
test_end

test_begin 'a .jit missing its objects directory is repaired'
  fresh partial-repo
  mkdir .jit
  run init
  assert_status 0
  assert_dir_exists .jit/objects
  assert_stdout_equals "Reinitialized existing Jit repository in $(jit_dir)"
test_end

# failure paths

test_begin '.jit existing as a regular file exits 128'
  fresh jit-is-a-file
  : > .jit
  run init
  assert_status 128
  assert_stderr_contains ".jit"
  assert_stdout_empty
test_end

if [ "$(id -u)" = "0" ]; then
  skip 'an unwritable working directory exits 128' 'running as root ignores mode bits'
else
  test_begin 'an unwritable working directory exits 128'
    fresh unwritable
    chmod 500 .
    run init
    chmod 700 .
    assert_status 128
    assert_stderr_contains 'Permission denied'
    assert_stdout_empty
  test_end
fi

# options and operands

test_begin 'operands are rejected with a warning but still initialize'
  fresh with-operands
  run init somewhere
  assert_status 0
  assert_dir_exists .jit/objects
  assert_stderr_contains 'does not take any arguments'
test_end

# init's option table is empty, which no other suite exercises.
test_begin '-h prints usage and exits 129'
  fresh dash-h
  run init -h
  assert_status 129
  assert_stderr_contains 'usage: jit init'
  assert_stdout_empty
  assert_equals '. ' "$(tree_here)" 'tree'
test_end

test_summary
