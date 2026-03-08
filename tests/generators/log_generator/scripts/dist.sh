#!/bin/bash

# Exit immediately if a command fails
set -e

echo "==> Changing to the script's root directory..."
cd "$(dirname "$0")"
echo "==> Current directory: $(pwd)"
echo ""

echo "==> Starting the Python dist script (release preset)..."
python3 run.py dist --preset release "$@"

echo ""
echo "==> Launcher script finished."
