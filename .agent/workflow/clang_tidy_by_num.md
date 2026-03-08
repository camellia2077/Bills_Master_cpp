---
description: Run scoped Tidy tasks for bills_tracer (by task count or batch count, bills task queue)
---

### Scope Mapping (MUST)
- Default tidy analysis scope is:
  - `apps/bills_cli/src`
  - `libs/bills_core/src`
  - `libs/bills_io/src`
- Optional non-default scope:
  - `tests/generators/log_generator/src`
- Scope source of truth is:
  - `tools/toolchain/config/workflow.toml` -> `[scope]`
- Inspect the resolved scope before bootstrap when needed:
  - `python tools/run.py tidy-scope`
- Task queue location is fixed to:
  - `temp/tidy/tasks/batch_*/task_*.log`
- Resume-first source of truth is:
  - `temp/tidy/tasks/manifest.json`
  - `temp/tidy/tidy_result.json`
- Keep incremental build from existing:
  - `build/bills/tidy/shared`; do not delete it.
- Verify gate command (manual spot-check):
  - `python tools/run.py verify bills-build`
- Verify gate state file used by strict tidy cleanup:
  - `temp/tidy/verify_result.json`
  - This file is written by `tidy-batch` / `tidy-close`, not by plain `python tools/run.py verify ...`.
- Tidy machine summary (single source for agent):
  - `temp/tidy/tidy_result.json`
  - Read this file first for `tasks.remaining`, `blocking_files`, and `next_action`.
- check -> fix_strategy rule table:
  - `tools/toolchain/config/workflow.toml` -> `[tidy.fix_strategy]`
  - Categories: `auto_fix`, `safe_refactor`, `nolint_allowed`, `manual_only`

### Python Execution Directory (MUST)
- All Python commands in this workflow must run from repository root:
  - repository root (`bills_tracer/`)
- Do not `cd` into `apps/bills_cli` before running `python tools/run.py ...`.

### Inputs (MUST)
- Choose exactly one mode:
  - **Task mode**: finish exactly `<TASK_N>` task logs by manual loop
  - **Batch mode**: finish exactly `<BATCH_N>` non-empty `batch_*` folders
- Default path is **Batch mode + tidy-batch**.
- **Task mode** is manual / troubleshooting oriented in current `bills_tracer`; there is no dedicated task-count auto loop yet.
- Batch mode target set = the smallest `<BATCH_N>` non-empty batches under `temp/tidy/tasks/`; freeze this set for the whole run.
- Fix order rule:
  - Always consume auto-fixable work first, then switch to manual log-driven fixing.
  - This order is mandatory.
  - The agent MUST NOT begin from manual task logs if `tidy-loop` can still close eligible batches automatically.

### Fix Cadence (MUST)
- Work one `task_NNN.log` at a time; do not batch-edit multiple logs before verification.
- After each task fix, a manual spot-check may use:
  - `python tools/run.py verify bills-build`
- For normal batch closure, use:
  - `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop --timeout-seconds 1800`
- `tidy-batch` is the unified path for verify gate + clean + incremental/full refresh.
- Optional knobs for longer runs or stricter closure:
  - `--full-every <N>`: force periodic full refresh cadence
  - `--run-verify`: force verify inside the batch close command
  - `--strict-clean`: require strict clean gate semantics
- If timeout or interruption occurs:
  - inspect `temp/tidy/tidy_batch_checkpoint.json`
  - rerun the same `tidy-batch` command for the same `BATCH_ID`
- `tidy-refresh` fallback (auto inside `tidy-batch`):
  - it currently auto switches to full tidy on:
    - `no such file`
    - `glob mismatch`
  - after auto rebuild, treat old queue assumptions as stale and continue only from the newest `temp/tidy/tasks/`

### Suppression Policy (MUST)
- Prefer fixing warnings over suppressing them.
- If suppression is unavoidable, use only narrow local suppression:
  - `NOLINTNEXTLINE`
  - `NOLINTBEGIN/END`
- Every suppression must carry a short reason comment.
- Do not add file-wide or directory-wide blanket suppression for:
  - `bugprone-*`
  - `readability-*`
  - `cppcoreguidelines-*`

### Refactor Guardrail (MUST)
- For long-file maintainability refactors:
  - Step 1: in-file boundary convergence first
  - Step 2: physical split to new `*.cpp` / `*.hpp` only after Step 1 is stable
- Do not mix behavior changes with structural tidy-only refactors in the same batch.

### Task Source
- If any `temp/tidy/tasks/batch_*/task_*.log` exists: resume only; do not regenerate.
- Only when tasks are missing (bootstrap once):
  - `python tools/run.py tidy --jobs 4 --keep-going`
  - `python tools/run.py tidy-split`

### Baseline Verify
- Before manual fix rounds, ensure the fast build path is healthy:
  - `python tools/run.py verify bills-build`
- For final authoritative batch gate, still use:
  - `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop`
  - add `--full-every <N>` when the run explicitly wants bounded incremental drift

### Auto-First Triage
- If the queue does not exist yet, bootstrap first:
  - `python tools/run.py tidy --jobs 4 --keep-going`
  - `python tools/run.py tidy-split`
- Before any manual task reading, run the auto phase for the requested scope:
  - full queue: `python tools/run.py tidy-loop --all --test-every 1 --concise`
  - bounded batch-count warmup: `python tools/run.py tidy-loop --n <BATCH_N> --test-every 1 --concise`
- Purpose:
  - clear checks covered by `tools/toolchain/config/workflow.toml` -> `[tidy.fix_strategy.auto_fix]`
  - reduce noise before humans inspect remaining logs
- Only after this phase finishes may the agent start reading `task_*.log` and doing manual fixes.
- Before this phase finishes, entering manual log-fix mode is prohibited.

### Auto Loop
- Batch mode may optionally warm up auto-fixable batches first:
  - `python tools/run.py tidy-loop --n <BATCH_N> --test-every 1 --concise`
- Current `bills_tracer` `tidy-loop` works in batch-count terms, not task-count terms.
- Exit code:
  - `0`: requested amount finished, no eligible auto-fix batch remains, or loop stopped at manual-only work
  - non-zero: stop and diagnose
- Interpretation:
  - `0` does not mean the whole tidy run is done.
  - It means the auto-fix-first phase is complete for the requested scope, and the remaining queue becomes the manual work set.
  - Only at this point may the agent switch to manual log-driven fixing.

### Single-Task Loop (one task each round)
- Pick one task:
  - Task mode: smallest global `task_NNN.log`
  - Batch mode: smallest `task_NNN.log` within the frozen selected batches
- Analyze only this log.
- Fix only this task (or one source-file cluster if multiple logs map to the same file).
- Manual verify:
  - `python tools/run.py verify bills-build`
- Do not manually run `clean + tidy-refresh` in normal flow.
- For multiple tasks mapped to the same source file in one batch, fallback clustered clean is:
  - `python tools/run.py clean --batch-id <BATCH_ID> --strict --cluster-by-file <TASK_ID>`
- After one batch is fully fixed, close it with the unified command:
  - `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop`
  - optional strict variant: `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop --strict-clean --run-verify`
- Manual fallback `clean` / `tidy-refresh` is only for troubleshooting command failures.

### Stop Conditions (MUST)
- Task mode done:
  - exactly `<TASK_N>` target task logs have been fixed and archived from the chosen scope
- Batch mode done:
  - the frozen selected `<BATCH_N>` batches are all empty / removed
- Final acceptance when this run is intended as full completion:
  - `python tools/run.py tidy-close --keep-going --concise`
  - `tidy-close` includes: `tidy-refresh --final-full` + `verify` + empty-`task_*.log` check
- Tidy-only acceptance:
  - `python tools/run.py tidy-close --tidy-only --keep-going`
  - this mode enforces only: `tidy-refresh --final-full` + empty-`task_*.log` check
