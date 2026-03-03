# C ABI Boundary (Phase 0 Baseline)

## Scope
- This document freezes the current refactor boundary for Android reuse.
- Source of truth for Phase 0/1 decisions in `docs/others/C_ABI_SCHEMA_DRAFT.md`.
- Working draft copy for discussion: `temp/c-abi.md`.

## Module Boundaries
- `libs/bills_core`
  - Holds business rules, validation/conversion pipeline, and application use-cases.
  - Must not contain platform-specific code (`_WIN32`, `LoadLibrary`, direct SQLite handles).
- `products/bills_cli`
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
- Build Windows CLI:
  - `python tools/verify/verify.py bills-build`
- Build + run CLI test suite:
  - `python tools/verify/verify.py bills`
- Build `bills_core` shared library:
  - `python tools/verify/verify.py core-build`
- Run `bills_core` C ABI smoke tests:
  - `python tools/verify/verify.py core-abi`

## Notes
- In Phase 1, `bills_core` is configurable as `STATIC/SHARED`.
- Default in CLI build remains static to avoid runtime behavior drift during migration.
