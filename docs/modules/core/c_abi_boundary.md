# C ABI Boundary (Phase 0 Baseline)

## Scope
- This document freezes the current refactor boundary for Android reuse.
- Source of truth for Phase 0/1 decisions in `docs/modules/core/c_abi_schema_draft.md`.
- Working draft copy for discussion: `temp/c-abi.md`.

## Module Boundaries
- `libs/core`
  - Holds business rules, validation/conversion pipeline, and application use-cases.
  - Must not contain platform-specific code (`_WIN32`, `LoadLibrary`, direct SQLite handles).
- `apps/bills_cli`
  - Holds CLI presentation, Windows infrastructure adapters, plugin loading, and report exporting.
  - Wires platform implementations into `bills_core` ports.

## Cross-Boundary Contract Rules
- Interop boundary target: C ABI.
- Payload format: UTF-8 JSON text.
- ABI response envelope baseline:
  - `ok` (bool)
  - `code` (string)
  - `message` (string)
  - `data` (object/array/null)
- Memory ownership must be explicit on every ABI string return.

## Phase 0 Baseline Verify Commands
- Prepare Windows CLI dist:
  - `python tools/run.py dist bills-tracer-cli --preset debug --scope shared`
- Prepare dist + run CLI test suite:
  - `python tools/run.py verify bills-tracer`
- Prepare `bills_core` static dist:
  - `python tools/run.py dist bills-tracer-core --preset debug`

## Notes
- Windows no longer keeps a shared-library verification path for `bills_core`.
- Default in CLI dist remains static to avoid runtime behavior drift during migration.
