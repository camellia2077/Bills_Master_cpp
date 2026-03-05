#!/bin/bash

# Exit immediately if a command fails
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

echo "==> Script directory: $SCRIPT_DIR"
echo "==> Repo root: $REPO_ROOT"
echo ""

echo "==> Starting the Python tidy script..."
python3 "$REPO_ROOT/tools/build/build_bills_master.py" tidy "$@"

echo ""
echo "==> Tidy script finished."
