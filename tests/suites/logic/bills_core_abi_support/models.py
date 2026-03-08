from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class AbiSchemaContext:
    abi_version: str
    response_schema_version: int
    error_code_schema_version: int
