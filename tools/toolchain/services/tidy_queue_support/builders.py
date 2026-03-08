from __future__ import annotations

from ..fix_strategy import summarize_checks
from ..task_sorter import calculate_priority_score
from ..tidy_log_parser import extract_diagnostics, generate_text_summary, group_sections


def split_log_to_tasks(
    ctx,
    *,
    log_content: str,
    max_lines: int | None = None,
    max_diags: int | None = None,
) -> list[dict]:
    effective_max_lines = ctx.config.tidy.max_lines if max_lines is None else max_lines
    effective_max_diags = ctx.config.tidy.max_diags if max_diags is None else max_diags
    if effective_max_lines <= 0 or effective_max_diags <= 0:
        raise ValueError("max_lines and max_diags must be > 0")

    tasks: list[dict] = []
    for section in group_sections(log_content.splitlines()):
        tasks.extend(
            _process_section(
                section,
                max_lines=effective_max_lines,
                max_diags=effective_max_diags,
                strategy_cfg=ctx.config.tidy.fix_strategy,
                safe_fix_patterns=ctx.config.tidy.safe_fix_prepass.checks,
                suppression_allowed_patterns=ctx.config.tidy.suppression.allowed_checks,
            )
        )
    tasks.sort(key=lambda item: (item["score"], item["size"]))
    return tasks


def _process_section(
    section_lines: list[str],
    *,
    max_lines: int,
    max_diags: int,
    strategy_cfg,
    safe_fix_patterns: list[str],
    suppression_allowed_patterns: list[str],
) -> list[dict]:
    diagnostics = extract_diagnostics(section_lines)
    if not diagnostics:
        return []

    tasks: list[dict] = []
    current_batch: list[dict] = []
    batch_lines = 0

    def finalize(batch: list[dict]) -> None:
        if not batch:
            return
        source_file = str(batch[0]["file"])
        checks = sorted({str(item["check"]) for item in batch})
        strategy_summary = summarize_checks(
            checks,
            strategy_cfg=strategy_cfg,
            safe_fix_patterns=safe_fix_patterns,
            suppression_allowed_patterns=suppression_allowed_patterns,
        )
        summary = generate_text_summary(batch)
        original_lines: list[str] = []
        for diagnostic in batch:
            original_lines.extend(diagnostic["lines"])
        content = (
            f"File: {source_file}\n"
            + "=" * 60
            + "\n"
            + summary
            + "\n".join(original_lines)
            + ("\n" if original_lines else "")
        )
        tasks.append(
            {
                "content": content,
                "size": len(content),
                "diagnostics": batch,
                "source_file": source_file,
                "checks": checks,
                "primary_fix_strategy": strategy_summary.primary_strategy,
                "safe_fix_checks_present": strategy_summary.safe_fix_checks_present,
                "suppression_candidates_present": strategy_summary.suppression_candidates_present,
                "score": calculate_priority_score(batch, source_file),
            }
        )

    for diagnostic in diagnostics:
        diagnostic_line_count = len(diagnostic["lines"])
        if current_batch and (
            len(current_batch) >= max_diags or batch_lines + diagnostic_line_count > max_lines
        ):
            finalize(current_batch)
            current_batch = []
            batch_lines = 0
        current_batch.append(diagnostic)
        batch_lines += diagnostic_line_count

    finalize(current_batch)
    return tasks
