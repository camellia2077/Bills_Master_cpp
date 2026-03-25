#!/usr/bin/env python3

from __future__ import annotations

import argparse
import ast
from dataclasses import dataclass
from pathlib import Path

IGNORE_DIR_NAMES = {
    ".git",
    ".idea",
    ".vscode",
    "__pycache__",
    "venv",
    ".venv",
    "env",
    ".env",
    "dist",
}


@dataclass(frozen=True)
class Violation:
    file_path: Path
    line: int
    import_name: str
    reason: str


def should_skip(path: Path) -> bool:
    return any(part in IGNORE_DIR_NAMES for part in path.parts)


def normalize_module_name(node: ast.AST) -> tuple[str, int] | None:
    if isinstance(node, ast.Import):
        if not node.names:
            return None
        return node.names[0].name, node.lineno
    if isinstance(node, ast.ImportFrom):
        if node.level > 0 or not node.module:
            return None
        return node.module, node.lineno
    return None


def detect_layer(relative_path: Path) -> str:
    normalized = relative_path.as_posix()
    if normalized.startswith("tools/flows/"):
        return "flows"
    if normalized.startswith("tools/verify/"):
        return "verify"
    return "other"


def check_import_rule(layer: str, import_name: str) -> str | None:
    if import_name.startswith(("apps", "libs", "tests")):
        return (
            "tools 脚本不应直接 import 业务层模块（apps/libs/tests）；应通过 CLI/子进程边界调用。"
        )
    if layer == "flows" and import_name.startswith("tools.verify"):
        return "tools/flows 不应依赖 tools/verify。"
    if layer == "verify" and import_name.startswith(("tools.build", "tools.flows")):
        return "tools/verify 不应直接依赖 tools/flows（应通过子进程调用脚本）。"
    return None


def scan_python_file(file_path: Path, repo_root: Path) -> list[Violation]:
    violations: list[Violation] = []
    relative_path = file_path.resolve().relative_to(repo_root.resolve())
    layer = detect_layer(relative_path)

    try:
        content = file_path.read_text(encoding="utf-8")
    except OSError as exc:
        violations.append(
            Violation(
                file_path=relative_path,
                line=1,
                import_name="<read-file>",
                reason=f"无法读取文件: {exc}",
            )
        )
        return violations

    try:
        tree = ast.parse(content, filename=str(file_path))
    except SyntaxError as exc:
        violations.append(
            Violation(
                file_path=relative_path,
                line=exc.lineno or 1,
                import_name="<syntax-error>",
                reason=f"语法错误: {exc.msg}",
            )
        )
        return violations

    for node in ast.walk(tree):
        info = normalize_module_name(node)
        if info is None:
            continue
        import_name, line = info
        reason = check_import_rule(layer, import_name)
        if reason is None:
            continue
        violations.append(
            Violation(
                file_path=relative_path,
                line=line,
                import_name=import_name,
                reason=reason,
            )
        )
    return violations


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Check layering/dependency rules for tools/* python scripts."
    )
    parser.add_argument(
        "--root",
        default="tools",
        help="Root directory to scan (default: tools).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[3]
    scan_root = (repo_root / args.root).resolve()

    if not scan_root.exists():
        print(f"[ERROR] Path does not exist: {scan_root}")
        return 2

    python_files = [
        path
        for path in scan_root.rglob("*.py")
        if path.is_file() and not should_skip(path.relative_to(repo_root))
    ]
    python_files.sort()

    violations: list[Violation] = []
    for file_path in python_files:
        violations.extend(scan_python_file(file_path, repo_root))

    if not violations:
        print(f"[OK] tools layering check passed. files={len(python_files)}")
        return 0

    print(
        f"[FAILED] tools layering check failed: "
        f"violations={len(violations)}, files={len(python_files)}"
    )
    for item in violations:
        print(
            f"- {item.file_path.as_posix()}:{item.line} "
            f"import '{item.import_name}' -> {item.reason}"
        )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
