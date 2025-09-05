#!/usr/bin/env python3

from build_utils import run_build_pipeline

def main():
    """
    为 Release 模式执行构建流程。
    """
    BUILD_TYPE = "Release"
    BUILD_DIR = "build"
    run_build_pipeline(BUILD_TYPE, BUILD_DIR)

if __name__ == "__main__":
    main()