from __future__ import annotations

from pathlib import Path


def _write_markdown_summary(tasks: list[dict], out_path: Path) -> None:
    lines = [
        "# Clang-Tidy Tasks Summary\n",
        "| ID | Batch | File | Difficulty Score | Warning Types | Fix Strategy |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for task in tasks:
        lines.append(
            "| {task_id} | {batch_id} | {source_file} | {score:.2f} | {checks} | {strategy} |".format(
                task_id=task["task_id"],
                batch_id=task["batch_id"],
                source_file=task["source_file"],
                score=float(task["score"]),
                checks=", ".join(task["checks"]),
                strategy=task["primary_fix_strategy"],
            )
        )
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines), encoding="utf-8")
