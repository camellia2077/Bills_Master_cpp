from __future__ import annotations

from .models import ImportRecord, IncludeRecord, Violation
from .scan import CALL_LAYER_ROOTS


def print_stats(
    include_records: list[IncludeRecord],
    import_records: list[ImportRecord],
    scanned_file_counts: dict[str, int],
) -> None:
    include_by_scope: dict[str, int] = {}
    quoted_include_by_scope: dict[str, int] = {}
    import_by_scope: dict[str, int] = {}

    for item in include_records:
        include_by_scope[item.scope] = include_by_scope.get(item.scope, 0) + 1
        if item.delimiter == '"':
            quoted_include_by_scope[item.scope] = quoted_include_by_scope.get(item.scope, 0) + 1
    for item in import_records:
        import_by_scope[item.scope] = import_by_scope.get(item.scope, 0) + 1

    print("[INFO] import/include distribution (call-layer roots)")
    print("scope,files,include_total,include_quoted,import_total")
    for scope_name in CALL_LAYER_ROOTS.keys():
        file_count = scanned_file_counts.get(scope_name, 0)
        include_total = include_by_scope.get(scope_name, 0)
        include_quoted = quoted_include_by_scope.get(scope_name, 0)
        import_total = import_by_scope.get(scope_name, 0)
        print(f"{scope_name},{file_count},{include_total},{include_quoted},{import_total}")


def check_policy(
    include_records: list[IncludeRecord],
    allowlist: set[tuple[str, str]],
) -> tuple[list[Violation], set[tuple[str, str]]]:
    violations: list[Violation] = []
    observed = set()

    for record in include_records:
        if record.delimiter != '"':
            continue

        key = (record.file_path.as_posix(), record.header)
        observed.add(key)
        if key in allowlist:
            continue

        violations.append(
            Violation(
                file_path=record.file_path,
                line=record.line,
                header=record.header,
                reason=(
                    "调用层新增了未登记的 quoted include。"
                    "请改为 import，或在 allowlist 中补充 owner/reason。"
                ),
            )
        )

    return violations, observed
