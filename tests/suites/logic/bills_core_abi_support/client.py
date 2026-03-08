from __future__ import annotations

import ctypes
import json
import os
import shutil
from pathlib import Path

from .build import load_windows_dll_search_dirs


class AbiClient:
    def __init__(self, library_path: Path) -> None:
        self._dll_dirs: list[object] = []
        if os.name == "nt":
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
