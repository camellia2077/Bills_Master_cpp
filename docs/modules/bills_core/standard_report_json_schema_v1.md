# Standard Report JSON Schema v1

## 1. Scope
- Purpose: provide a unified report data contract before rendering to MD/LaTeX/JSON.
- Applies to: monthly report and yearly report.
- Source of truth: bills_core report DTO.
- Positioning: JSON is one renderer output of `StandardReport`, not an independent business source.

## 1.1 Ownership & Boundary
- `StandardReport` owner: `libs/bills_core/src/reporting/standard_report/standard_report_dto.hpp`
- Assembler boundary: `*ReportData -> StandardReport`
- Serializer boundary: `StandardReport -> JSON string`
- Renderer boundary: output-specific rendering (`md/tex/json`) belongs to presentation/export layer

## 2. Top-Level Structure
- `meta` object (required)
- `scope` object (required)
- `summary` object (required)
- `items` object (required)
- `extensions` object (required, may be empty)

## 3. Field Contract
### 3.1 `meta`
- `schema_version` string, required, current `1.0.0`
- `report_type` string, required, enum: `monthly` | `yearly`
- `generated_at_utc` string, required, ISO8601 UTC (`YYYY-MM-DDTHH:MM:SSZ`)
- `source` string, required, default `bills_core`

### 3.2 `scope`
- `period_start` string, required, `YYYY-MM`
- `period_end` string, required, `YYYY-MM`
- `remark` string, optional (empty string allowed)
- `data_found` bool, required

### 3.3 `summary`
- `total_income` number, required
- `total_expense` number, required
- `balance` number, required

### 3.4 `items`
- `categories` array, required, monthly uses this
  - item:
    - `name` string
    - `total` number
    - `sub_categories` array
      - item:
        - `name` string
        - `subtotal` number
        - `transactions` array
          - item:
            - `parent_category` string
            - `sub_category` string
            - `transaction_type` string
            - `description` string
            - `source` string
            - `comment` string
            - `amount` number
- `monthly_summary` array, required, yearly uses this
  - item:
    - `month` int (1-12)
    - `income` number
    - `expense` number
    - `balance` number

### 3.5 `extensions`
- reserved for non-breaking additions
- must be object

## 4. Consistency Rules
- money values are stored as JSON `number`; renderers display with fixed 2 decimals.
- `period_start`/`period_end` use `YYYY-MM`; `generated_at_utc` uses UTC `YYYY-MM-DDTHH:MM:SSZ`.
- yearly `items.monthly_summary` is sorted by month ascending (1..12).
- monthly category/sub-category/transaction ordering follows stable amount-desc order in renderer path.
- optional text fields use empty string defaults; list fields use empty arrays when no data.
- serializer key order is stable for snapshot reproducibility.
- renderer should consume `items.monthly_summary[].balance` as contract value; recomputing in renderer is transitional behavior to be removed in Struct-First phase 2.

## 5. Compatibility Policy
- additive fields only within `1.x` and should be optional.
- removing or changing field semantics requires `2.0.0`.

## 6. Minimal Example
```json
{
  "meta": {
    "schema_version": "1.0.0",
    "report_type": "monthly",
    "generated_at_utc": "2026-01-01T00:00:00Z",
    "source": "bills_core"
  },
  "scope": {
    "period_start": "2024-01",
    "period_end": "2024-01",
    "remark": "",
    "data_found": false
  },
  "summary": {
    "total_income": 0.0,
    "total_expense": 0.0,
    "balance": 0.0
  },
  "items": {
    "categories": [],
    "monthly_summary": []
  },
  "extensions": {}
}
```

## 7. Full Example (Monthly)
```json
{
  "meta": {
    "schema_version": "1.0.0",
    "report_type": "monthly",
    "generated_at_utc": "2026-01-01T00:00:00Z",
    "source": "bills_core"
  },
  "scope": {
    "period_start": "2024-01",
    "period_end": "2024-01",
    "remark": "",
    "data_found": true
  },
  "summary": {
    "total_income": 13389.61,
    "total_expense": -1220.59,
    "balance": 12169.02
  },
  "items": {
    "categories": [
      {
        "name": "meal",
        "total": -897.23,
        "sub_categories": [
          {
            "name": "meal_low",
            "subtotal": -401.82,
            "transactions": [
              {
                "parent_category": "meal",
                "sub_category": "meal_low",
                "transaction_type": "",
                "description": "饭",
                "source": "",
                "comment": "",
                "amount": -401.82
              }
            ]
          }
        ]
      }
    ],
    "monthly_summary": []
  },
  "extensions": {}
}
```

## 8. Full Example (Yearly)
```json
{
  "meta": {
    "schema_version": "1.0.0",
    "report_type": "yearly",
    "generated_at_utc": "2026-01-01T00:00:00Z",
    "source": "bills_core"
  },
  "scope": {
    "period_start": "2024-01",
    "period_end": "2024-12",
    "remark": "",
    "data_found": true
  },
  "summary": {
    "total_income": 145582.91,
    "total_expense": -16350.18,
    "balance": 129232.73
  },
  "items": {
    "categories": [],
    "monthly_summary": [
      {
        "month": 1,
        "income": 13389.61,
        "expense": -1220.59,
        "balance": 12169.02
      },
      {
        "month": 2,
        "income": 14618.69,
        "expense": -1199.54,
        "balance": 13419.15
      }
    ]
  },
  "extensions": {}
}
```
