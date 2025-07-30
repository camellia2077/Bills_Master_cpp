#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

echo "==> Changing to script's directory..."
# This command ensures we are in the same directory as the script.
cd "$(dirname "$0")"
echo "==> Current directory: $(pwd)"
echo ""

echo "==> Executing Python build script..."
# Run the Python build script. Using python3 is recommended.
python3 build_debug.py

echo ""
echo "==> Wrapper script finished."