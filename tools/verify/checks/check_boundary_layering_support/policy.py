from __future__ import annotations

from .models import IncludeRecord, Violation
from .scan import BOUNDARY_LAYER_ROOTS


def classify_entry(file_path: str, header: str) -> tuple[str, str]:
    if (
        file_path.startswith("libs/io/src/io/adapters/")
        or header == "nlohmann/json.hpp"
        or header == "abi/bills_core_abi.h"
    ):
        return (
            "long-term",
            "长期保留（第三方/平台适配或 ABI 对外契约，不计划在迁移窗口内移除）",
        )

    if file_path.startswith("libs/core/src/modules/"):
        return (
            "replaceable",
            "Phase 5.2（编译器兼容矩阵稳定后，评估 header-unit/纯 import 替代桥接 include）",
        )

    return (
        "replaceable",
        "Phase 5.1（模块默认 ON 并保留 OFF 回退期后，评估继续压缩 ABI 边界 include）",
    )


def reason_for_entry(file_path: str, header: str, tier: str) -> str:
    if tier == "long-term":
        if header == "abi/bills_core_abi.h":
            return "C ABI 对外导出头，属于稳定边界契约。"
        if header == "nlohmann/json.hpp":
            return "JSON 协议解析依赖第三方库，边界层直接保留。"
        if file_path.startswith("libs/io/src/io/adapters/"):
            return "IO 适配器属于平台/第三方边界实现，允许保留 include。"
        return "边界层长期依赖，短期内无替换收益。"

    if file_path.startswith("libs/core/src/modules/"):
        return "模块桥接段用于连接遗留头与 export 接口，后续可继续收敛。"
    if file_path.startswith("libs/core/src/abi/"):
        return "ABI 命令处理与共享边界实现当前需要该 include，后续继续迁移为更小边界。"
    return "边界层临时依赖，后续在迁移窗口内持续压缩。"


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
                    "边界层新增了未登记的 quoted include。"
                    "请补充 allowlist 的 owner/reason/window/tier。"
                ),
            )
        )

    return violations, observed


def print_stats(
    include_records: list[IncludeRecord],
    scanned_file_counts: dict[str, int],
    observed: set[tuple[str, str]] | None = None,
    tier_map: dict[tuple[str, str], str] | None = None,
) -> None:
    include_by_scope: dict[str, int] = {}
    quoted_include_by_scope: dict[str, int] = {}

    for item in include_records:
        include_by_scope[item.scope] = include_by_scope.get(item.scope, 0) + 1
        if item.delimiter == '"':
            quoted_include_by_scope[item.scope] = quoted_include_by_scope.get(item.scope, 0) + 1

    print("[INFO] include distribution (boundary roots)")
    print("scope,files,include_total,include_quoted")
    for scope_name in BOUNDARY_LAYER_ROOTS.keys():
        file_count = scanned_file_counts.get(scope_name, 0)
        include_total = include_by_scope.get(scope_name, 0)
        include_quoted = quoted_include_by_scope.get(scope_name, 0)
        print(f"{scope_name},{file_count},{include_total},{include_quoted}")

    if observed is not None and tier_map is not None:
        long_term = sum(1 for key in observed if tier_map.get(key) == "long-term")
        replaceable = sum(1 for key in observed if tier_map.get(key) == "replaceable")
        print(
            "[INFO] boundary allowlist tier stats: "
            f"replaceable={replaceable}, long-term={long_term}"
        )
