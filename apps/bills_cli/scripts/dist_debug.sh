#!/bin/bash

# Exit immediately if a command fails
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

echo "==> Script directory: $SCRIPT_DIR"
echo "==> Repo root: $REPO_ROOT"
echo ""

echo "==> Starting the Python dist script (debug preset)..."
python3 "$REPO_ROOT/tools/run.py" dist bills --preset debug --scope shared "$@"

echo ""
echo "==> Dist script finished."
