# C ABI Command & Schema Draft (for Android Reuse)

Status: Draft  
Last updated: 2026-02-27  
Primary implementation reference: `apps/bills_core/src/abi/bills_core_abi.cpp`

## 1. ABI Surface

```c
const char* bills_core_get_abi_version();
const char* bills_core_get_capabilities_json();
const char* bills_core_invoke_json(const char* request_json_utf8);
void bills_core_free_string(const char* owned_utf8_str);
```

## 2. Memory Ownership Contract

1. `bills_core_get_abi_version()`
   - Returns static internal string.
   - Caller must NOT free.
2. `bills_core_get_capabilities_json()`
   - Returns heap-owned UTF-8 string (malloc).
   - Caller must call `bills_core_free_string`.
3. `bills_core_invoke_json(...)`
   - Returns heap-owned UTF-8 string (malloc).
   - Caller must call `bills_core_free_string`.
4. `bills_core_free_string(...)`
   - Compatible with null pointer (`free(nullptr)` semantics).

## 3. JSON Request Envelope

All requests are UTF-8 JSON text.

```json
{
  "command": "validate | convert | ingest | import | query | capabilities | ping | version",
  "params": {},
  "payload": {}
}
```

Rules:
- `command`: required, non-empty string.
- `params`: optional, defaults to `{}`; must be object when present.
- `payload`: optional, used by `ping` for echo.

## 4. JSON Response Envelope

All `bills_core_invoke_json` responses use:

```json
{
  "ok": true,
  "code": "ok",
  "message": "human readable",
  "data": {},
  "error_layer": "none | param | business | system",
  "abi_version": "1.0.0",
  "response_schema_version": 2,
  "error_code_schema_version": 1
}
```

Layer mapping:
- `ok` => `error_layer = none`
- `param.*` => `param`
- `business.*` => `business`
- `system.*` (or unknown) => `system`

## 5. Versioning Fields

- `abi_version = "1.0.0"`
- `response_schema_version = 2`
- `capabilities_schema_version = 1`
- `error_code_schema_version = 1`

## 6. Error Code Registry (Draft)

### 6.1 Parameter / Request Errors
- `param.invalid_argument`
- `param.invalid_json`
- `param.invalid_request`
- `param.unknown_command`
- `param.invalid_config`
- `param.invalid_input_path`

### 6.2 Business Errors
- `business.no_input_files`
- `business.validation_failed`
- `business.convert_failed`
- `business.ingest_failed`
- `business.import_failed`
- `business.query_failed`
- `business.query_not_found`

### 6.3 System Errors
- `system.not_implemented` (reserved in current draft)

## 7. Config Input Rules (Shared by validate/convert/ingest)

Exactly one of the following modes should be supplied:

1. Inline config mode:
   - `params.validator_config` (object)
   - `params.modifier_config` (object)
2. Directory mode:
   - `params.config_dir` (string)
   - Core reads:
     - `${config_dir}/Validator_Config.json`
     - `${config_dir}/Modifier_Config.json`
3. Explicit path mode:
   - `params.validator_config_path` (string)
   - `params.modifier_config_path` (string)

If none of the above is valid, return `param.invalid_config`.

### 7.1 validator_config shape

```json
{
  "categories": [
    {
      "parent_item": "string",
      "sub_items": ["string"]
    }
  ]
}
```

### 7.2 modifier_config shape

```json
{
  "auto_renewal_rules": {
    "enabled": true,
    "rules": [
      {
        "header_location": "string",
        "amount": 0.0,
        "description": "string"
      }
    ]
  },
  "metadata_prefixes": ["string"],
  "display_name_maps": {
    "category_key": {
      "lang_key": "display_name"
    }
  }
}
```

## 8. Command Schemas (Draft)

## 8.1 `version`

### Request
```json
{
  "command": "version"
}
```

### Response `data`
```json
{
  "abi_version": "1.0.0",
  "response_schema_version": 2,
  "capabilities_schema_version": 1,
  "error_code_schema_version": 1
}
```

## 8.2 `capabilities`

### Request
```json
{
  "command": "capabilities"
}
```

### Response `data`
```json
{
  "capabilities": {
    "abi_version": "1.0.0",
    "capabilities_schema_version": 1,
    "response_schema_version": 2,
    "error_code_schema_version": 1,
    "supported_commands": [],
    "wired_commands": [],
    "error_layers": [],
    "error_code_examples": {},
    "notes": []
  }
}
```

## 8.3 `ping`

### Request
```json
{
  "command": "ping",
  "payload": {}
}
```

### Response `data`
```json
{
  "pong": true,
  "abi_version": "1.0.0",
  "response_schema_version": 2,
  "echo": {}
}
```

## 8.4 `validate`

### Request
```json
{
  "command": "validate",
  "params": {
    "input_path": "path/to/txt-or-dir",
    "config_dir": "path/to/config-dir"
  }
}
```

`input_path` rules:
- Must be existing file/dir.
- File mode requires `.txt` extension.
- Directory mode recursively enumerates `.txt`.

### Success/Failure Response `data`
```json
{
  "input_path": "string",
  "processed": 0,
  "success": 0,
  "failure": 0,
  "all_valid": true,
  "files": [
    {
      "path": "string",
      "ok": true,
      "error": "optional string"
    }
  ]
}
```

## 8.5 `convert`

### Request
```json
{
  "command": "convert",
  "params": {
    "input_path": "path/to/txt-or-dir",
    "output_dir": "output/txt2josn",
    "write_files": true,
    "include_serialized_json": false,
    "config_dir": "path/to/config-dir"
  }
}
```

Defaults:
- `output_dir = "output/txt2josn"` (kept for compatibility)
- `write_files = true`
- `include_serialized_json = false`

### Response `data`
```json
{
  "input_path": "string",
  "output_dir": "string",
  "write_files": true,
  "processed": 0,
  "success": 0,
  "failure": 0,
  "all_converted": true,
  "files": [
    {
      "path": "string",
      "ok": true,
      "output_path": "optional string",
      "json": "optional serialized bill json string",
      "error": "optional string"
    }
  ]
}
```

## 8.6 `ingest`

### Request
```json
{
  "command": "ingest",
  "params": {
    "input_path": "path/to/txt-or-dir",
    "output_dir": "output/txt2josn",
    "write_json": false,
    "include_serialized_json": false,
    "config_dir": "path/to/config-dir"
  }
}
```

Defaults:
- `output_dir = "output/txt2josn"`
- `write_json = false`
- `include_serialized_json = false`

### Response `data`
```json
{
  "input_path": "string",
  "output_dir": "string",
  "write_json": false,
  "processed": 0,
  "success": 0,
  "failure": 0,
  "imported": 0,
  "repository_mode": "memory",
  "all_ingested": true,
  "files": [
    {
      "path": "string",
      "ok": true,
      "output_path": "optional string",
      "json": "optional serialized bill json string",
      "error": "optional string"
    }
  ]
}
```

## 8.7 `import`

### Request
```json
{
  "command": "import",
  "params": {
    "input_path": "path/to/json-or-dir"
  }
}
```

`input_path` rules:
- Must be existing file/dir.
- File mode requires `.json` extension.
- Directory mode recursively enumerates `.json`.

### Response `data`
```json
{
  "input_path": "string",
  "processed": 0,
  "success": 0,
  "failure": 0,
  "imported": 0,
  "repository_mode": "memory",
  "all_imported": true,
  "files": [
    {
      "path": "string",
      "ok": true,
      "date": "optional string",
      "year": "optional int",
      "month": "optional int",
      "transaction_count": "optional int",
      "error": "optional string"
    }
  ]
}
```

## 8.8 `query`

### Request
```json
{
  "command": "query",
  "params": {
    "type": "year | y | month | m",
    "value": "YYYY or YYYYMM",
    "input_path": "output/txt2josn"
  }
}
```

Defaults:
- `input_path = "output/txt2josn"`

Validation rules:
- `type = year|y` -> `value` must be `YYYY` and range `[1900, 9999]`.
- `type = month|m` -> `value` must be `YYYYMM`, month `[01, 12]`.

### Response `data`
```json
{
  "input_path": "string",
  "query_type": "year | month",
  "query_value": "string",
  "year": 2024,
  "month": 1,
  "processed": 0,
  "parse_failures": 0,
  "matched_bills": 0,
  "transaction_count": 0,
  "total_income": 0.0,
  "total_expense": 0.0,
  "balance": 0.0,
  "category_totals": {
    "income": {},
    "expense": {}
  },
  "monthly_summary": [
    {
      "month": 1,
      "income": 0.0,
      "expense": 0.0,
      "balance": 0.0
    }
  ],
  "files": [
    {
      "path": "string",
      "ok": true,
      "matched": true,
      "date": "optional string",
      "year": "optional int",
      "month": "optional int",
      "transaction_count": "optional int",
      "error": "optional string"
    }
  ]
}
```

Notes:
- `monthly_summary` is returned only for year query.
- If no matched records: `business.query_not_found`.
- If partial parse errors exist: `business.query_failed` (even with matched data).

## 9. Serialized Bill JSON (for `convert`/`ingest` optional inline output)

When `include_serialized_json=true`, per-file `json` field contains stringified:

```json
{
  "date": "YYYYMM",
  "remark": "string",
  "total_income": 0.0,
  "total_expense": 0.0,
  "balance": 0.0,
  "categories": {
    "ParentCategory": {
      "display_name": "ParentCategory",
      "sub_total": 0.0,
      "transactions": [
        {
          "sub_category": "string",
          "description": "string",
          "amount": 0.0,
          "source": "string",
          "transaction_type": "string",
          "comment": "string or null"
        }
      ]
    }
  }
}
```

## 10. Current Limitations (must be explicit to Android caller)

1. `ingest` currently writes to in-memory repository only.
2. `import` currently writes to in-memory repository only.
3. `query` currently reads JSON files from path, not DB-backed query port.
4. Most command inputs are path-first (not memory-first yet).

## 11. Compatibility Policy (Draft)

1. Backward-compatible field additions:
   - Allowed in `data`.
2. Breaking changes:
   - Require `abi_version` or schema version bump.
3. Error code stability:
   - Existing code identifiers should not be renamed in patch/minor updates.
