#!/bin/bash

# Exit immediately if a command fails
set -e

echo "==> Changing to the script's root directory..."
cd "$(dirname "$0")"
echo "==> Current directory: $(pwd)"
echo ""

echo "==> Starting the Python dist script (debug preset)..."
python3 run.py dist --preset debug "$@"

echo ""
echo "==> Dist launcher finished."
