#!/bin/bash

# Exit immediately if a command fails
set -e

echo "==> Changing to the script's root directory..."
# This command ensures the script runs from its own location
cd "$(dirname "$0")"
echo "==> Current directory: $(pwd)"
echo ""

echo "==> Starting the Python build script..."
# Execute the Python script
echo "==> Starting the Python build script..."
# Execute the Python script
# Execute the Python script
python3 run.py build --mode Release "$@"

echo ""
echo "==> Launcher script finished."