#!/bin/bash

# Exit immediately if a command fails
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

echo "==> Script directory: $SCRIPT_DIR"
echo "==> Repo root: $REPO_ROOT"
echo ""

echo "==> Starting the Python fast build script (Debug)..."
python3 "$REPO_ROOT/tools/flows/build_bills_master.py" --preset debug --scope shared "$@"

echo ""
echo "==> Fast build finished."
