from __future__ import annotations

from .models import AbiSchemaContext


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def require_response_envelope(
    response: dict,
    schema: AbiSchemaContext,
    *,
    expected_ok: bool | None = None,
    expected_code: str | None = None,
    expected_error_layer: str | None = None,
) -> None:
    required_keys = [
        "ok",
        "code",
        "message",
        "data",
        "error_layer",
        "abi_version",
        "response_schema_version",
        "error_code_schema_version",
    ]
    for key in required_keys:
        require(key in response, f"response missing required envelope field: {key}")

    require(isinstance(response.get("ok"), bool), "response.ok must be bool.")
    require(isinstance(response.get("code"), str), "response.code must be string.")
    require(isinstance(response.get("message"), str), "response.message must be string.")
    require(
        isinstance(response.get("error_layer"), str),
        "response.error_layer must be string.",
    )

    if expected_ok is not None:
        require(response.get("ok") is expected_ok, f"response.ok must be {expected_ok}.")
    if expected_code is not None:
        require(
            response.get("code") == expected_code,
            f"response.code must be '{expected_code}'.",
        )
    if expected_error_layer is not None:
        require(
            response.get("error_layer") == expected_error_layer,
            f"response.error_layer must be '{expected_error_layer}'.",
        )

    require(
        response.get("abi_version") == schema.abi_version,
        "response.abi_version mismatch.",
    )
    require(
        response.get("response_schema_version") == schema.response_schema_version,
        "response.response_schema_version mismatch.",
    )
    require(
        response.get("error_code_schema_version") == schema.error_code_schema_version,
        "response.error_code_schema_version mismatch.",
    )
