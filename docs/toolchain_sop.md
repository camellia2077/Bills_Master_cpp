# bills_tracer Toolchain SOP

## Entry

- Unified entry: `python tools/run.py`
- Toolchain config: `tools/toolchain/config/workflow.toml`
- Runtime artifacts: `temp/format/` and `temp/tidy/`

## Recommended Flow

1. `python tools/run.py format --check`
2. `python tools/run.py tidy`
3. `python tools/run.py tidy-split`
4. `python tools/run.py tidy-next`
5. `python tools/run.py tidy-batch --batch-id <BATCH_ID> --preset sop`
6. Repeat step 4-5 until no pending batches remain
7. `python tools/run.py tidy-close`

## Agent Helpers

- List all pending batches: `python tools/run.py tidy-list`
- Show one batch: `python tools/run.py tidy-show --batch-id <BATCH_ID>`
- Archive verified tasks directly: `python tools/run.py clean --batch-id <BATCH_ID> --strict`
- Refresh queue after one batch: `python tools/run.py tidy-refresh --batch-id <BATCH_ID>`
- Run `clang-tidy -fix` for a batch: `python tools/run.py tidy-fix --batch-id <BATCH_ID>`
- Loop through auto-fixable batches: `python tools/run.py tidy-loop --all`

## Notes

- `tidy-batch --preset sop` expands to strict clean + verify + concise output + `full_every=3` + keep-going.
- `tidy-refresh` prefers incremental refresh, and automatically upgrades to full refresh when incremental scope is unavailable or log anomalies require it.
- `tidy-close` is the global close command: it runs a final refresh, optional verify, and then requires the pending task queue to be empty.
