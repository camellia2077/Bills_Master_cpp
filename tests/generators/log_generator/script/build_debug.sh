#!/bin/bash

# Exit immediately if a command fails
set -e

echo "==> Changing to the script's root directory..."
cd "$(dirname "$0")"
echo "==> Current directory: $(pwd)"
echo ""

echo "==> Starting the Python build script (Debug)..."
python3 run.py build --mode Debug "$@"

echo ""
echo "==> Wrapper script finished."
