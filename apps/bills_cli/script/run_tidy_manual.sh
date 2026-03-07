#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

for arg in "$@"; do
  case "$arg" in
    -h|--help)
      python3 "$REPO_ROOT/tools/run.py" tidy "$@"
      exit 0
      ;;
  esac
done

set +e
python3 "$REPO_ROOT/tools/run.py" tidy "$@"
tidy_exit_code=$?
set -e

python3 "$REPO_ROOT/tools/run.py" tidy-split
split_exit_code=$?

if [ "$split_exit_code" -ne 0 ]; then
  exit "$split_exit_code"
fi

exit "$tidy_exit_code"
