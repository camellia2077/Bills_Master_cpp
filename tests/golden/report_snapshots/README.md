# Report Snapshot Golden

This directory stores frozen golden files for report regression checks.

Covered scenarios:
- Monthly report: `2025-01`
- Yearly report: `2025`
- Date range reports: `2025-03`, `2025-04`

Covered formats:
- Markdown output (`Markdown_bills/...`)
- Standard JSON render output (`standard_json/...`)
- StandardReport canonical JSON golden (`standard_report/...`)
- LaTeX output (`LaTeX_bills/...`)
- reStructuredText output (`reST_bills/...`)
- Typst source output (`Typst_bills/...`)

Scripts:
- Freeze/update baseline:
  - `python tools/verify/freeze_report_snapshots.py`
- Validate current outputs:
  - `python tools/verify/check_report_snapshots.py`

Notes:
- Markdown snapshots are compared by parsed content using `markdown-it-py`, not by raw bytes.
- JSON snapshots are compared by parsed JSON content using Python's built-in `json` library, not by raw bytes.
- `tex` / `typ` / `rst` snapshots are compared byte-for-byte on purpose.
- These rules are intentional. Do not "simplify" them back to one compare mode without updating both tooling and this note.
- Unified verify entry can run the full fixture export + golden checks:
  - `python tools/run.py verify bills-tracer`
