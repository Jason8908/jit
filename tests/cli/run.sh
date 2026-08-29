#!/bin/sh
# Runs every tests/cli/t*.sh suite and aggregates their results.

set -u

dir="$(cd "$(dirname "$0")" && pwd)"
fail=0

for suite in "$dir"/t*.sh; do
  [ -f "$suite" ] || continue
  echo "== tests/cli/$(basename "$suite")"
  sh "$suite" || fail=1
done

exit $fail
