# Report Snapshot Baseline

This directory stores frozen baseline files for report regression checks.

Covered scenarios:
- Monthly report: `2024-01`
- Yearly report: `2024`
- Date range reports: `2024-03`, `2024-04`

Covered formats:
- Markdown output (`Markdown_bills/...`)
- Standard JSON output (`standard_json/...`)

Scripts:
- Freeze/update baseline:
  - `python scripts/freeze_report_snapshots.py`
- Validate current outputs:
  - `python scripts/check_report_snapshots.py`

Notes:
- JSON comparison ignores `meta.generated_at_utc`.
