#!/usr/bin/env python3

import sys
import os

# Ensure we can import modules from current directory
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

# Ensure we can import modules from current directory
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from automation.config_loader import load_config
from automation.cli import main

if __name__ == "__main__":
    config = load_config()
    main(config)
