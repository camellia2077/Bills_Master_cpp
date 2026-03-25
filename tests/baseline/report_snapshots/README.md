# Report Snapshot Baseline

This directory stores frozen baseline files for report regression checks.

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
- Render JSON snapshot comparison ignores `meta.generated_at_utc` but keeps renderer field order.
- StandardReport golden comparison ignores `meta.generated_at_utc` and compares canonicalized JSON.
- Unified verify entry can run the full fixture export + golden checks:
  - `python tools/run.py verify bills-tracer`
