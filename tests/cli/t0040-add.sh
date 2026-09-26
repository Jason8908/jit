#!/bin/sh
# End-to-end tests for `jit add` (src/cmd_add.c, src/dir.c).

. "$(dirname "$0")/harness.sh"

OID_HELLO_WORLD='95d09f2b10159347eece71399a7e2e907ea3df4f'

# fresh <name> -- move into an empty initialized repository.
fresh() {
  cd "$TRASH" || exit 1
  rm -rf "$1"
  mkdir "$1" || exit 1
  cd "$1" || exit 1
  "$JIT" init >/dev/null 2>&1 || exit 1
}

count_objects() { find .jit/objects -type f | wc -l | tr -d ' '; }

# obj_path <oid> -- the loose object path an oid is stored at.
obj_path() {
  printf '.jit/objects/%s/%s' "$(printf '%s' "$1" | cut -c1-2)" \
                              "$(printf '%s' "$1" | cut -c3-)"
}

assert_object_count() {
  got="$(count_objects)"
  [ "$got" = "$1" ] || fail "object count: want $1, got $got"
}

assert_file_exists() {
  [ -f "$1" ] || fail "want file '$1', found none"
}

# The deflated encoding of a blob containing 'hello world', from real git.
HELLO_WORLD_Z='\170\001\113\312\311\117\122\060\064\144\310\110\315\311\311\127\050\317\057\312\111\001\000\075\173\006\176'

assert_no_temp_files() {
  got="$(find .jit/objects -name 'tmp_*' | wc -l | tr -d ' ')"
  [ "$got" = "0" ] || fail "temp files left behind: $got"
}

# assert_starts_with_zlib_header <file>
assert_starts_with_zlib_header() {
  got="$(od -An -tx1 -N2 "$1" | tr -d ' \n')"
  [ "$got" = "7801" ] || fail "$1: want zlib level-1 header 7801, got $got"
}

# files

test_begin 'adds a single file and stores it at its oid'
  fresh single
  printf '%s' 'hello world' > hw.txt
  run add hw.txt
  assert_status 0
  assert_file_exists "$(obj_path "$OID_HELLO_WORLD")"
  assert_object_count 1
  assert_stdout_empty
  assert_stderr_empty
  assert_no_temp_files
test_end

# compression

test_begin 'the stored object is deflated at level 1'
  fresh compressed
  printf '%s' 'hello world' > hw.txt
  run add hw.txt
  assert_status 0
  assert_starts_with_zlib_header "$(obj_path "$OID_HELLO_WORLD")"
test_end

test_begin 'the stored bytes match what git writes'
  fresh git-bytes
  printf '%s' 'hello world' > hw.txt
  run add hw.txt
  assert_status 0
  printf "$HELLO_WORLD_Z" > want.z
  cmp -s want.z "$(obj_path "$OID_HELLO_WORLD")" || fail 'stored bytes differ from git'
test_end

test_begin 'the stored object inflates back to its encoded form'
  fresh inflate-back
  printf '%s' 'hello world' > hw.txt
  run add hw.txt
  assert_status 0
  if command -v python3 >/dev/null 2>&1; then
    got="$(python3 -c "import zlib,sys;sys.stdout.write(zlib.decompress(open(sys.argv[1],'rb').read()).decode('utf-8','replace'))" "$(obj_path "$OID_HELLO_WORLD")")"
    assert_equals "blob 11$(printf '\000')hello world" "$got" 'inflated object'
  else
    skip 'inflates back to its encoded form' 'python3 not available'
  fi
test_end

test_begin 'objects are stored read-only'
  fresh readonly-objects
  printf '%s' 'hello world' > hw.txt
  run add hw.txt
  assert_status 0
  if [ "$(id -u)" = "0" ]; then
    skip 'mode check' 'running as root ignores mode bits'
  else
    assert_perms 'r--r--r--' "$(obj_path "$OID_HELLO_WORLD")"
  fi
test_end

test_begin 're-adding an existing object succeeds despite the read-only file'
  fresh readd-readonly
  printf '%s' 'hello world' > hw.txt
  run add hw.txt
  run add hw.txt
  assert_status 0
  assert_stderr_empty
  assert_object_count 1
  assert_no_temp_files
test_end

test_begin 'object mode ignores the umask, directory mode honours it'
  fresh umask-modes
  old_umask="$(umask)"
  umask 077
  printf '%s' 'hello world' > hw.txt
  run add hw.txt
  umask "$old_umask"
  assert_status 0
  if [ "$(id -u)" = "0" ]; then
    skip 'mode check' 'running as root ignores mode bits'
  else
    assert_perms 'rwx------' .jit/objects/95
    assert_perms 'r--r--r--' "$(obj_path "$OID_HELLO_WORLD")"
  fi
test_end

test_begin 'a failed add leaves no temporary files'
  fresh failed-add-litter
  run add nope.txt
  assert_status 1
  assert_no_temp_files
test_end

test_begin 'adds several files at once'
  fresh several
  echo one > a.txt
  echo two > b.txt
  run add a.txt b.txt
  assert_status 0
  assert_object_count 2
  assert_stderr_empty
test_end

# directories

test_begin 'adds every regular file under a directory'
  fresh dir-basic
  mkdir -p src/deep
  echo one > src/a.txt
  echo two > src/deep/b.txt
  echo three > src/deep/c.txt
  run add src
  assert_status 0
  assert_object_count 3
  assert_stderr_empty
test_end

test_begin 'adds the whole working tree with a dot'
  fresh dir-dot
  mkdir -p a/b
  echo one > top.txt
  echo two > a/mid.txt
  echo three > a/b/deep.txt
  run add .
  assert_status 0
  assert_object_count 3
  assert_stderr_empty
test_end

test_begin 'does not hash the object store into itself'
  fresh dir-self
  echo one > a.txt
  run add .
  first="$(count_objects)"
  run add .
  assert_status 0
  assert_equals "$first" "$(count_objects)" 'object count after a second add'
  assert_equals '1' "$first" 'object count'
test_end

test_begin 'the repository directory is rejected as an operand'
  fresh dir-reject
  echo one > a.txt
  run add .
  before="$(count_objects)"
  for target in .jit .jit/objects ./.jit .jit/; do
    run add "$target"
    assert_status 1
    assert_stderr_contains 'inside the repository directory'
  done
  assert_equals "$before" "$(count_objects)" 'object count'
test_end

test_begin 'trailing slashes and dot components name the same tree'
  fresh dir-spelling
  mkdir -p src/deep
  echo one > src/a.txt
  echo two > src/deep/b.txt
  for target in src src/ ./src src/.; do
    run add "$target"
    assert_status 0
  done
  assert_object_count 2
  assert_stderr_empty
test_end

test_begin 'an empty directory adds nothing and succeeds'
  fresh dir-empty
  mkdir hollow
  run add hollow
  assert_status 0
  assert_object_count 0
  assert_stderr_empty
test_end

test_begin 'files and directories can be mixed as operands'
  fresh dir-mixed
  mkdir sub
  echo one > loose.txt
  echo two > sub/nested.txt
  run add loose.txt sub
  assert_status 0
  assert_object_count 2
test_end

test_begin 'identical contents in different files store one object'
  fresh dir-dedup
  mkdir sub
  printf '%s' 'hello world' > a.txt
  printf '%s' 'hello world' > sub/b.txt
  run add .
  assert_status 0
  assert_object_count 1
  assert_file_exists "$(obj_path "$OID_HELLO_WORLD")"
test_end

# skip rules

test_begin 'a nested repository directory is skipped'
  fresh dir-nested-repo
  mkdir -p sub/.jit
  echo kept > sub/kept.txt
  echo stored > sub/.jit/stored
  run add .
  assert_status 0
  assert_object_count 1
test_end

# symlinks

test_begin 'a symlink inside a tree is warned about and skipped'
  fresh link-in-tree
  echo real > real.txt
  ln -s real.txt link.txt
  run add .
  assert_status 0
  assert_object_count 1
  assert_stderr_contains 'skipping symlink'
  assert_stderr_contains 'link.txt'
test_end

test_begin 'a symlinked directory is not descended into'
  fresh link-to-dir
  mkdir real
  echo behind > real/behind.txt
  ln -s real link
  run add .
  assert_status 0
  assert_object_count 1
test_end

test_begin 'a symlink loop terminates'
  fresh link-loop
  echo real > real.txt
  ln -s b a
  ln -s a b
  run add .
  assert_status 0
  assert_object_count 1
test_end

test_begin 'a symlink named on the command line is an error'
  fresh link-operand
  echo real > real.txt
  ln -s real.txt link.txt
  run add link.txt
  assert_status 1
  assert_stderr_contains 'not a regular file'
  assert_object_count 0
test_end

# errors: report and continue

test_begin 'a missing path is reported and exits 1'
  fresh missing
  run add nope.txt
  assert_status 1
  assert_stderr_contains 'nope.txt'
  assert_object_count 0
test_end

test_begin 'good operands are still added alongside a missing one'
  fresh missing-mixed
  echo one > good.txt
  run add nope.txt good.txt
  assert_status 1
  assert_stderr_contains 'nope.txt'
  assert_object_count 1
test_end

test_begin 'an empty path is reported and exits 1'
  fresh empty-operand
  run add ''
  assert_status 1
  [ -s "$ERR" ] || fail 'stderr: want a message, got none'
test_end

if [ "$(id -u)" = "0" ]; then
  skip 'an unreadable file is reported but its siblings are added' 'running as root ignores mode bits'
  skip 'an unreadable directory is skipped but the walk continues' 'running as root ignores mode bits'
else
  test_begin 'an unreadable file is reported but its siblings are added'
    fresh unreadable-file
    echo one > good.txt
    echo two > secret.txt
    chmod 000 secret.txt
    run add .
    chmod 600 secret.txt
    assert_status 1
    assert_stderr_contains 'secret.txt'
    assert_object_count 1
  test_end

  test_begin 'an unreadable directory is skipped but the walk continues'
    fresh unreadable-dir
    mkdir locked
    echo inside > locked/inside.txt
    echo sibling > sibling.txt
    chmod 000 locked
    run add .
    chmod 700 locked
    assert_status 1
    assert_stderr_contains 'locked'
    assert_object_count 1
  test_end
fi

# awkward names

test_begin 'file names with spaces, format specifiers and dashes are added'
  fresh awkward
  echo one > 'sp ace.txt'
  echo two > '%s.txt'
  echo three > ./-n.txt
  run add .
  assert_status 0
  assert_object_count 3
  assert_stderr_empty
test_end

# usage and repository checks

test_begin 'no operands prints usage and exits 129'
  fresh no-operands
  run add
  assert_status 129
  assert_stderr_contains 'usage: jit add'
  assert_stdout_empty
test_end

test_begin '-h prints usage and exits 129'
  fresh dash-h
  run add -h
  assert_status 129
  assert_stderr_contains 'usage: jit add'
test_end

test_begin 'the usage line mentions directories'
  fresh usage-line
  run add -h
  assert_stderr_contains '<file|directory>'
test_end

test_begin 'running outside a repository exits 128'
  cd "$TRASH" || exit 1
  rm -rf outside
  mkdir outside || exit 1
  cd outside || exit 1
  echo one > a.txt
  run add a.txt
  assert_status 128
  assert_stderr_contains 'not a jit repository'
test_end

test_summary
