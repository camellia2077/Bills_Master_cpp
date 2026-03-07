---
description: Run all Tidy tasks for bills_tracer (bills task queue)
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
  - `apps/bills_cli/build_fast`; do not delete it.
- Verify gate command (manual spot-check):
  - `python tools/run.py verify bills-build -- build_fast`
- Verify gate state file used by strict tidy cleanup:
  - `temp/tidy/verify_result.json`
  - This file is written by `tidy-batch` / `tidy-close`, not by plain `python tools/run.py verify ...`.
- Tidy machine summary (single source for agent):
  - `temp/tidy/tidy_result.json`
  - Read this file first for `tasks.remaining`, `blocking_files`, and `next_action`.
- Latest raw tidy run:
  - `temp/tidy/raw/summary.json`
  - `temp/tidy/raw/build.log`
  - `temp/tidy/raw/diagnostics.jsonl`
  - Historical snapshots:
    - `temp/tidy/runs/<run_id>/raw/build.log`
    - `temp/tidy/runs/<run_id>/raw/diagnostics.jsonl`
    - `temp/tidy/runs/<run_id>/summary.json`
- check -> fix_strategy rule table:
  - `tools/toolchain/config/workflow.toml` -> `[tidy.fix_strategy]`
  - Categories: `auto_fix`, `safe_refactor`, `nolint_allowed`, `manual_only`

### Python Execution Directory (MUST)
- All Python commands in this workflow must run from repository root:
  - repository root (`bills_tracer/`)
- Do not `cd` into `apps/bills_cli` before running `python tools/run.py ...`.

### Entry Sequence (MUST)
- Resume-first rule:
  - If any `temp/tidy/tasks/batch_*/task_*.log` exists, resume only; do not regenerate the queue.
- Bootstrap only when task logs are missing:
  - `python tools/run.py tidy --jobs 4 --keep-going`
  - `python tools/run.py tidy-split`
- Mandatory ordering rule:
  - The agent MUST run the auto-fix phase first.
  - The agent MUST NOT start manual `task_*.log` fixing while auto-fixable work still exists in the queue.
  - Manual log-driven fixing is allowed only after the auto phase has finished for the current queue.
- Auto phase:
  - `python tools/run.py tidy-loop --all --test-every 1 --concise`
- `tidy-loop` exit code rule in current `bills_tracer` implementation:
  - `0`: auto phase finished for now; manual batches may still remain, so completion gate below still applies
  - non-zero: stop and diagnose
- Auto-first interpretation:
  - `tidy-loop` is the preferred first pass because it only targets checks classified as auto-fixable by `tools/toolchain/config/workflow.toml` -> `[tidy.fix_strategy.auto_fix]`.
  - This phase is meant to remove low-risk mechanical fixes before humans read remaining task logs.
- After auto phase, inspect remaining queue:
  - `python tools/run.py tidy-list`
  - `python tools/run.py tidy-next`
- After auto phase, switch to manual batch loop:
  - `python tools/run.py tidy-next`
  - `python tools/run.py tidy-show --batch-id <BATCH_ID>`
  - fix one task at a time
  - close one batch with:
    - `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop --timeout-seconds 1800`
  - if the run needs deterministic full-refresh cadence, add:
    - `--full-every <N>`

### Hard Completion Gate (MUST)
- Completion is valid only when no `task_*.log` exists under `temp/tidy/tasks/`.
- All `batch_*` folders under `temp/tidy/tasks/` must be empty or removed.
- Recommended final acceptance command:
  - `python tools/run.py tidy-close --keep-going --concise`
- In full close mode, `temp/tidy/verify_result.json` must exist and keep `"success": true`.
- Partial progress is not completion.

### Tidy-Only Close (MUST)
- For decoupled tidy close without business verify:
  - `python tools/run.py tidy-close --tidy-only --keep-going`
- For full close with verify:
  - `python tools/run.py tidy-close --keep-going --concise`

### Execution Rules (MUST)
- Queue bootstrap:
  - Only regenerate with `tidy` + `tidy-split` when the pending queue is absent.
  - If `temp/tidy/tasks/manifest.json` already contains pending tasks, resume from that queue.
- Fix ordering:
  - Phase 1 = auto-fixable checks first.
  - Phase 2 = manual-only / safe-refactor / remaining log-driven fixes.
  - This order is mandatory.
  - The only exception is diagnosing a compiler error that prevents the queue from being usable at all.
- Manual batch loop:
  - Use `python tools/run.py tidy-next` to pick the next open batch.
  - Use `python tools/run.py tidy-show --batch-id <BATCH_ID>` before editing.
  - Work one `task_NNN.log` at a time inside the chosen batch.
  - After each fix, a manual spot-check may use:
    - `python tools/run.py verify bills-build -- build_fast`
  - Do not treat the direct verify command as the strict-clean state source; the authoritative strict-clean verify file is still `temp/tidy/verify_result.json`.
  - When one batch is ready to close, use:
    - `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop --timeout-seconds 1800`
  - Optional batch-close knobs for long or strict runs:
    - `--full-every <N>`: force periodic full refresh after every `<N>` batches
    - `--run-verify`: force verify inside the batch close command
    - `--strict-clean`: require strict clean gate semantics
- Unified close path:
  - `tidy-batch` is the normal path for verify gate + clean + incremental/full refresh.
  - Do not manually chain `clean + tidy-refresh` in normal flow unless diagnosing command failures.
- Manual-fix entry condition:
  - Start reading and fixing raw task logs only after `tidy-loop` has finished consuming the current auto-fixable work.
  - Before that point, the agent is not allowed to enter manual log-fix mode.
  - Remaining queue after that point is the authoritative manual work set.
- Timeout / interruption:
  - Inspect `temp/tidy/tidy_batch_checkpoint.json`
  - Then rerun the same `tidy-batch` command against the same `BATCH_ID`
  - Do not assume perfect checkpoint resume beyond the current queue/state files
- Auto rebuild fallback:
  - `tidy-refresh` auto switches to full tidy on stale-graph signals currently implemented in this repo:
    - `no such file`
    - `glob mismatch`
  - After a full rebuild, always treat old mental context as stale and continue only from the newest queue in `temp/tidy/tasks/`
- Command inventory for agent use:
  - list queue: `python tools/run.py tidy-list`
  - next batch: `python tools/run.py tidy-next`
  - show batch: `python tools/run.py tidy-show --batch-id <BATCH_ID>`
  - batch close: `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop`
  - batch close with cadence: `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop --full-every <N>`
  - direct strict clean fallback: `python tools/run.py clean --batch-id <BATCH_ID> --strict --cluster-by-file <TASK_ID>`
  - direct refresh fallback: `python tools/run.py tidy-refresh --batch-id <BATCH_ID>`
  - direct fix helper: `python tools/run.py tidy-fix --batch-id <BATCH_ID>`
  - final close: `python tools/run.py tidy-close --keep-going --concise`

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
- Non-boundary implementation files should be fixed directly, not silenced.

### Refactor Guardrail (MUST)
- For long-file maintainability refactors:
  - Step 1: do in-file boundary convergence first
  - Step 2: split into new `*.cpp` / `*.hpp` only after Step 1 is stable
- Do not mix behavior changes with structural tidy-only refactors in the same batch.

### Stop Conditions (MUST)
- Stop immediately if:
  - `python tools/run.py tidy-batch ...` returns non-zero
  - `python tools/run.py tidy-close ...` returns non-zero
  - `python tools/run.py verify bills-build -- build_fast` returns non-zero during manual inspection
- Continue only while the queue in `temp/tidy/tasks/` is still valid and the next action in `temp/tidy/tidy_result.json` still points to the current batch progression.
