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
echo "==> Starting the Python build script (Debug)..."
# Execute the Python script
# Execute the Python script
python3 run.py build --mode Debug "$@"

echo ""
echo "==> Wrapper script finished."