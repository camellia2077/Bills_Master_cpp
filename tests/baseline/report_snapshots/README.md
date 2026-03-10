# Report Snapshot Baseline

This directory stores frozen baseline files for report regression checks.

Covered scenarios:
- Monthly report: `2025-01`
- Yearly report: `2025`
- Date range reports: `2025-03`, `2025-04`

Covered formats:
- Markdown output (`Markdown_bills/...`)
- Standard JSON output (`standard_json/...`)
- Typst source output (`Typst_bills/...`)

Scripts:
- Freeze/update baseline:
  - `python tools/verify/freeze_report_snapshots.py`
- Validate current outputs:
  - `python tools/verify/check_report_snapshots.py`

Notes:
- JSON comparison ignores `meta.generated_at_utc`.
