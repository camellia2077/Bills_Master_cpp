#!/bin/bash

# Exit immediately if a command fails
set -e

echo "==> Changing to the script's root directory..."
cd "$(dirname "$0")"
echo "==> Current directory: $(pwd)"
echo ""

echo "==> Starting the Python build script (Release)..."
python3 run.py build --mode Release "$@"

echo ""
echo "==> Launcher script finished."
