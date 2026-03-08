#!/usr/bin/env python3

from __future__ import annotations

import argparse
import ctypes
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.toolchain.core.config import load_toolchain_config
from tools.toolchain.services.build_layout import resolve_build_directory


WORKFLOW_TOML_PATH = REPO_ROOT / "tools" / "toolchain" / "config" / "workflow.toml"


def run_build(build_preset: str) -> None:
    command = [
        sys.executable,
        str(REPO_ROOT / "tools" / "flows" / "build_bills_core.py"),
        "--preset",
        build_preset,
        "--shared",
    ]
    print(f"==> Running: {' '.join(command)}")
    subprocess.run(command, check=True, cwd=REPO_ROOT)


def detect_library_path(build_preset: str) -> Path:
    base_dir = resolve_build_directory(
        REPO_ROOT,
        target="core",
        preset=build_preset,
        scope="shared",
    ).build_dir / "bin"
    if sys.platform.startswith("win"):
        candidates = ["bills_core.dll", "libbills_core.dll"]
    elif sys.platform == "darwin":
        candidates = ["libbills_core.dylib", "bills_core.dylib"]
    else:
        candidates = ["libbills_core.so", "bills_core.so"]

    for candidate in candidates:
        path = base_dir / candidate
        if path.is_file():
            return path

    names = ", ".join(candidates)
    raise FileNotFoundError(
        f"Cannot find bills_core shared library under '{base_dir}'. "
        f"Tried: {names}"
    )


class AbiClient:
    def __init__(self, library_path: Path) -> None:
        self._dll_dirs: list[object] = []
        if sys.platform.startswith("win"):
            self._register_windows_dll_dirs(library_path)
        self._lib = ctypes.CDLL(str(library_path))

        self._lib.bills_core_get_abi_version.restype = ctypes.c_char_p

        self._lib.bills_core_get_capabilities_json.argtypes = []
        self._lib.bills_core_get_capabilities_json.restype = ctypes.c_void_p

        self._lib.bills_core_invoke_json.argtypes = [ctypes.c_char_p]
        self._lib.bills_core_invoke_json.restype = ctypes.c_void_p

        self._lib.bills_core_free_string.argtypes = [ctypes.c_void_p]
        self._lib.bills_core_free_string.restype = None

    def abi_version(self) -> str:
        raw = self._lib.bills_core_get_abi_version()
        if raw is None:
            raise RuntimeError("bills_core_get_abi_version returned null.")
        return raw.decode("utf-8")

    def capabilities(self) -> dict:
        pointer = self._lib.bills_core_get_capabilities_json()
        return self._take_owned_json(pointer)

    def invoke_null(self) -> dict:
        pointer = self._lib.bills_core_invoke_json(None)
        return self._take_owned_json(pointer)

    def invoke_text(self, request_json_utf8: str) -> dict:
        payload = request_json_utf8.encode("utf-8")
        pointer = self._lib.bills_core_invoke_json(payload)
        return self._take_owned_json(pointer)

    def invoke(self, request_obj: dict) -> dict:
        request_json = json.dumps(request_obj, ensure_ascii=False)
        return self.invoke_text(request_json)

    def _take_owned_json(self, pointer: int) -> dict:
        if not pointer:
            raise RuntimeError("ABI returned null pointer for owned string.")
        try:
            text = ctypes.string_at(pointer).decode("utf-8")
            return json.loads(text)
        finally:
            self._lib.bills_core_free_string(pointer)

    def _register_windows_dll_dirs(self, library_path: Path) -> None:
        candidates: list[Path] = [library_path.parent]

        gpp_path = shutil.which("g++.exe")
        if gpp_path:
            candidates.append(Path(gpp_path).resolve().parent)

        candidates.extend(load_windows_dll_search_dirs())

        seen: set[str] = set()
        for candidate in candidates:
            if not candidate.is_dir():
                continue
            key = str(candidate).lower()
            if key in seen:
                continue
            seen.add(key)
            try:
                self._dll_dirs.append(os.add_dll_directory(str(candidate)))
            except (FileNotFoundError, OSError):
                continue


def load_windows_dll_search_dirs() -> list[Path]:
    config = load_toolchain_config(WORKFLOW_TOML_PATH)
    resolved: list[Path] = []
    for item in config.verify.windows.dll_search_dirs:
        candidate = Path(item.strip())
        if not str(candidate).strip():
            continue
        if not candidate.is_absolute():
            candidate = (WORKFLOW_TOML_PATH.parent / candidate).resolve()
        resolved.append(candidate)
    return resolved


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def select_fixture_txt_path() -> Path:
    fixture_root = REPO_ROOT / "tests" / "fixtures" / "bills"
    candidates = sorted(fixture_root.rglob("*.txt"))
    require(candidates, f"No .txt fixtures found under: {fixture_root}")
    return candidates[0]


def parse_iso_month_from_stem(path: Path) -> tuple[int, int, str]:
    stem = path.stem
    parts = stem.split("-")
    require(len(parts) == 2, f"Fixture file must be YYYY-MM.txt: {path}")
    require(parts[0].isdigit() and len(parts[0]) == 4, f"Invalid fixture year: {path}")
    require(parts[1].isdigit() and len(parts[1]) == 2, f"Invalid fixture month: {path}")
    year = int(parts[0])
    month = int(parts[1])
    require(1900 <= year <= 9999, f"Invalid year parsed from fixture: {path}")
    require(1 <= month <= 12, f"Invalid month parsed from fixture: {path}")
    return year, month, stem


def require_response_envelope(
    response: dict,
    *,
    expected_ok: bool | None = None,
    expected_code: str | None = None,
    expected_error_layer: str | None = None,
    expected_abi_version: str | None = None,
    expected_response_schema_version: int | None = None,
    expected_error_code_schema_version: int | None = None,
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
    if expected_abi_version is not None:
        require(
            response.get("abi_version") == expected_abi_version,
            "response.abi_version mismatch.",
        )
    if expected_response_schema_version is not None:
        require(
            response.get("response_schema_version") == expected_response_schema_version,
            "response.response_schema_version mismatch.",
        )
    if expected_error_code_schema_version is not None:
        require(
            response.get("error_code_schema_version") == expected_error_code_schema_version,
            "response.error_code_schema_version mismatch.",
        )


def run_fixture_command_tests(
    client: AbiClient,
    *,
    abi_version: str,
    response_schema_version: int,
    error_code_schema_version: int,
) -> None:
    fixture_config_dir = REPO_ROOT / "tests" / "config"
    require(fixture_config_dir.is_dir(), f"Fixture config dir missing: {fixture_config_dir}")

    fixture_txt = select_fixture_txt_path()
    year, month, iso_month = parse_iso_month_from_stem(fixture_txt)

    validate = client.invoke(
        {
            "command": "validate",
            "params": {
                "input_path": str(fixture_txt),
                "config_dir": str(fixture_config_dir),
            },
        }
    )
    require_response_envelope(
        validate,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    validate_data = validate.get("data", {})
    require(validate_data.get("processed") == 1, "validate.processed must be 1.")
    require(validate_data.get("all_valid") is True, "validate.all_valid must be true.")
    print("[PASS] validate fixture.")

    with tempfile.TemporaryDirectory(prefix="bills_core_abi_") as temp_dir:
        output_dir = Path(temp_dir) / "txt2json"

        convert = client.invoke(
            {
                "command": "convert",
                "params": {
                    "input_path": str(fixture_txt),
                    "output_dir": str(output_dir),
                    "write_files": True,
                    "include_serialized_json": False,
                    "config_dir": str(fixture_config_dir),
                },
            }
        )
        require_response_envelope(
            convert,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
            expected_abi_version=abi_version,
            expected_response_schema_version=response_schema_version,
            expected_error_code_schema_version=error_code_schema_version,
        )
        convert_data = convert.get("data", {})
        require(convert_data.get("processed") == 1, "convert.processed must be 1.")
        require(convert_data.get("all_converted") is True, "convert.all_converted must be true.")

        convert_files = convert_data.get("files", [])
        require(isinstance(convert_files, list) and len(convert_files) == 1, "convert.files must have 1 item.")
        output_path_raw = convert_files[0].get("output_path", "")
        require(isinstance(output_path_raw, str) and output_path_raw, "convert.files[0].output_path must exist.")
        output_path = Path(output_path_raw)
        require(output_path.is_file(), f"convert output file must exist: {output_path}")
        print("[PASS] convert fixture and wrote JSON.")

        ingest = client.invoke(
            {
                "command": "ingest",
                "params": {
                    "input_path": str(fixture_txt),
                    "output_dir": str(output_dir),
                    "write_json": False,
                    "include_serialized_json": False,
                    "config_dir": str(fixture_config_dir),
                },
            }
        )
        require_response_envelope(
            ingest,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
            expected_abi_version=abi_version,
            expected_response_schema_version=response_schema_version,
            expected_error_code_schema_version=error_code_schema_version,
        )
        ingest_data = ingest.get("data", {})
        require(ingest_data.get("processed") == 1, "ingest.processed must be 1.")
        require(ingest_data.get("all_ingested") is True, "ingest.all_ingested must be true.")
        require(ingest_data.get("imported") == 1, "ingest.imported must be 1.")
        require(ingest_data.get("repository_mode") == "memory", "ingest.repository_mode must be 'memory'.")
        print("[PASS] ingest fixture.")

        imported = client.invoke(
            {
                "command": "import",
                "params": {
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            imported,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
            expected_abi_version=abi_version,
            expected_response_schema_version=response_schema_version,
            expected_error_code_schema_version=error_code_schema_version,
        )
        import_data = imported.get("data", {})
        require(import_data.get("processed") == 1, "import.processed must be 1.")
        require(import_data.get("all_imported") is True, "import.all_imported must be true.")
        require(import_data.get("imported") == 1, "import.imported must be 1.")
        require(import_data.get("repository_mode") == "memory", "import.repository_mode must be 'memory'.")

        import_files = import_data.get("files", [])
        require(isinstance(import_files, list) and len(import_files) == 1, "import.files must have 1 item.")
        require(import_files[0].get("ok") is True, "import.files[0].ok must be true.")
        require(import_files[0].get("year") == year, "import.files[0].year mismatch.")
        require(import_files[0].get("month") == month, "import.files[0].month mismatch.")
        print("[PASS] import fixture.")

        query_year = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "year",
                    "value": str(year),
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            query_year,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
            expected_abi_version=abi_version,
            expected_response_schema_version=response_schema_version,
            expected_error_code_schema_version=error_code_schema_version,
        )
        qy_data = query_year.get("data", {})
        require(qy_data.get("query_type") == "year", "query_year.query_type must be 'year'.")
        require(qy_data.get("year") == year, "query_year.year mismatch.")
        require(qy_data.get("processed") == 1, "query_year.processed must be 1.")
        require(qy_data.get("matched_bills") == 1, "query_year.matched_bills must be 1.")
        require("monthly_summary" in qy_data, "query_year.monthly_summary must exist.")
        print("[PASS] query year fixture.")

        query_month = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "month",
                    "value": iso_month,
                    "input_path": str(output_path),
                },
            }
        )
        require_response_envelope(
            query_month,
            expected_ok=True,
            expected_code="ok",
            expected_error_layer="none",
            expected_abi_version=abi_version,
            expected_response_schema_version=response_schema_version,
            expected_error_code_schema_version=error_code_schema_version,
        )
        qm_data = query_month.get("data", {})
        require(qm_data.get("query_type") == "month", "query_month.query_type must be 'month'.")
        require(qm_data.get("year") == year, "query_month.year mismatch.")
        require(qm_data.get("month") == month, "query_month.month mismatch.")
        require(qm_data.get("processed") == 1, "query_month.processed must be 1.")
        require(qm_data.get("matched_bills") == 1, "query_month.matched_bills must be 1.")
        require("monthly_summary" not in qm_data, "query_month.monthly_summary must not exist.")
        print("[PASS] query month fixture.")


def run_tests(client: AbiClient, smoke_loops: int) -> None:
    abi_version = client.abi_version()
    require(bool(abi_version), "abi_version must be non-empty.")
    print("[PASS] abi_version is non-empty.")

    caps = client.capabilities()
    require(caps.get("abi_version") == abi_version, "capabilities.abi_version mismatch.")
    response_schema_version = caps.get("response_schema_version")
    error_code_schema_version = caps.get("error_code_schema_version")
    require(
        isinstance(response_schema_version, int),
        "capabilities.response_schema_version must be int.",
    )
    require(
        isinstance(error_code_schema_version, int),
        "capabilities.error_code_schema_version must be int.",
    )
    commands = caps.get("supported_commands", [])
    require(isinstance(commands, list), "capabilities.supported_commands must be list.")
    for required in ["version", "capabilities", "ping", "validate", "convert", "ingest", "import", "query"]:
        require(required in commands, f"Missing command in capabilities: {required}")
    require(
        caps.get("error_layers") == ["none", "param", "business", "system"],
        "capabilities.error_layers mismatch.",
    )
    print("[PASS] capabilities includes required commands.")

    version = client.invoke({"command": "version"})
    require_response_envelope(
        version,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    version_data = version.get("data", {})
    require(version_data.get("abi_version") == abi_version, "version.abi_version mismatch.")
    require(
        version_data.get("response_schema_version") == response_schema_version,
        "version.data.response_schema_version mismatch.",
    )
    require(
        version_data.get("error_code_schema_version") == error_code_schema_version,
        "version.data.error_code_schema_version mismatch.",
    )
    print("[PASS] version response schema.")

    null_input = client.invoke_null()
    require_response_envelope(
        null_input,
        expected_ok=False,
        expected_code="param.invalid_argument",
        expected_error_layer="param",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    print("[PASS] null request handling.")

    invalid_json = client.invoke_text("{")
    require_response_envelope(
        invalid_json,
        expected_ok=False,
        expected_code="param.invalid_json",
        expected_error_layer="param",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    print("[PASS] invalid JSON handling.")

    unknown = client.invoke({"command": "nonexistent_command"})
    require_response_envelope(
        unknown,
        expected_ok=False,
        expected_code="param.unknown_command",
        expected_error_layer="param",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    print("[PASS] unknown command handling.")

    invalid_params = client.invoke({"command": "validate", "params": []})
    require_response_envelope(
        invalid_params,
        expected_ok=False,
        expected_code="param.invalid_request",
        expected_error_layer="param",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    print("[PASS] invalid params type handling.")

    with tempfile.TemporaryDirectory(prefix="bills_core_abi_empty_") as empty_dir:
        business = client.invoke(
            {
                "command": "query",
                "params": {
                    "type": "year",
                    "value": "2024",
                    "input_path": str(Path(empty_dir)),
                },
            }
        )
    require_response_envelope(
        business,
        expected_ok=False,
        expected_code="business.no_input_files",
        expected_error_layer="business",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    print("[PASS] business error layer handling.")

    ping_payload = {"hello": "world", "n": 7}
    ping = client.invoke({"command": "ping", "payload": ping_payload})
    require_response_envelope(
        ping,
        expected_ok=True,
        expected_code="ok",
        expected_error_layer="none",
        expected_abi_version=abi_version,
        expected_response_schema_version=response_schema_version,
        expected_error_code_schema_version=error_code_schema_version,
    )
    require(ping.get("data", {}).get("pong") is True, "ping response must contain pong=true.")
    require(
        ping.get("data", {}).get("echo") == ping_payload,
        "ping echo payload mismatch.",
    )
    print("[PASS] ping echo contract.")

    run_fixture_command_tests(
        client,
        abi_version=abi_version,
        response_schema_version=response_schema_version,
        error_code_schema_version=error_code_schema_version,
    )

    for _ in range(smoke_loops):
        _ = client.capabilities()
        _ = client.invoke({"command": "version"})
        _ = client.invoke({"command": "ping", "payload": {"loop": True}})
        _ = client.invoke_text("{")
        _ = client.invoke({"command": "nonexistent_command"})
    print(f"[PASS] ownership/free smoke test ({smoke_loops} loops, mixed responses).")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run C ABI compatibility smoke tests for libs/bills_core."
    )
    parser.add_argument(
        "--preset",
        choices=["debug", "release"],
        default="debug",
        help="Build preset used when --skip-build is not set.",
    )
    parser.add_argument(
        "--lib",
        dest="library_path",
        default="",
        help="Explicit shared library path. If omitted, auto-detect from build dir.",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip invoking tools/flows/build_bills_core.py.",
    )
    parser.add_argument(
        "--smoke-loops",
        type=int,
        default=200,
        help="Loop count for ownership/free smoke test.",
    )
    args = parser.parse_args()

    try:
        if not args.skip_build:
            run_build(args.preset)

        if args.library_path:
            library_path = Path(args.library_path).resolve()
            if not library_path.is_file():
                raise FileNotFoundError(f"Library not found: {library_path}")
        else:
            library_path = detect_library_path(args.preset)

        print(f"==> Testing ABI library: {library_path}")
        client = AbiClient(library_path)
        run_tests(client, smoke_loops=args.smoke_loops)
    except Exception as exc:  # pylint: disable=broad-except
        print(f"[FAILED] {exc}")
        return 1

    print("[OK] bills_core ABI smoke tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
