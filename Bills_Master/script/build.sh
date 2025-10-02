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
python3 build.py

echo ""
echo "==> Launcher script finished."