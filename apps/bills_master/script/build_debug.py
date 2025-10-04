#!/usr/bin/env python3

from build_utils import run_build_pipeline

def main():
    """
    为 Debug 模式执行构建流程。
    """
    BUILD_TYPE = "Debug"
    BUILD_DIR = "build_debug"
    run_build_pipeline(BUILD_TYPE, BUILD_DIR)

if __name__ == "__main__":
    main()