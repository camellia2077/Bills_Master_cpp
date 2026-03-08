# C ABI Boundary (Phase 0 Baseline)

## Scope
- This document freezes the current refactor boundary for Android reuse.
- Source of truth for Phase 0/1 decisions in `docs/modules/bills_core/c_abi_schema_draft.md`.
- Working draft copy for discussion: `temp/c-abi.md`.

## Module Boundaries
- `libs/bills_core`
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
  - `python tools/verify/verify.py bills-dist`
- Prepare dist + run CLI test suite:
  - `python tools/verify/verify.py bills`
- Prepare `bills_core` shared library dist:
  - `python tools/verify/verify.py core-dist`
- Run `bills_core` C ABI smoke tests:
  - `python tools/verify/verify.py core-abi`

## Notes
- In Phase 1, `bills_core` is configurable as `STATIC/SHARED`.
- Default in CLI dist remains static to avoid runtime behavior drift during migration.
