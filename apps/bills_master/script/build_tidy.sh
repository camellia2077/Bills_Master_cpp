#!/bin/bash

# Exit immediately if a command fails
set -e

echo "==> Changing to the script's root directory..."
# This command ensures the script runs from its own location
cd "$(dirname "$0")"
echo "==> Current directory: $(pwd)"
echo ""

echo "==> Starting the Python tidy script..."
echo "==> Starting the Python tidy script..."
python3 run.py tidy

echo ""
echo "==> Tidy script finished."
