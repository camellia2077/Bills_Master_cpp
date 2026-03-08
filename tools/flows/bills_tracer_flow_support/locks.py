from __future__ import annotations

import json
import os
import shutil
import time
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path

LOCK_OWNER_FILENAME = ".lock_owner.json"
ORPHAN_LOCK_RECLAIM_SECONDS = 120


@contextmanager
def directory_lock(
    lock_dir: Path,
    timeout_seconds: int = 900,
    orphan_reclaim_seconds: int = ORPHAN_LOCK_RECLAIM_SECONDS,
):
    lock_dir = lock_dir.resolve()
    lock_parent = lock_dir.parent
    lock_parent.mkdir(parents=True, exist_ok=True)
    start = time.time()

    def owner_file() -> Path:
        return lock_dir / LOCK_OWNER_FILENAME

    def read_owner() -> dict | None:
        path = owner_file()
        if not path.exists():
            return None
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return None

    def write_owner() -> None:
        payload = {
            "pid": os.getpid(),
            "created_at": datetime.now().isoformat(timespec="seconds"),
        }
        try:
            owner_file().write_text(
                json.dumps(payload, ensure_ascii=False, indent=2),
                encoding="utf-8",
            )
        except OSError:
            pass

    def is_process_alive(pid: int) -> bool:
        if pid <= 0:
            return False
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return False
        except PermissionError:
            return True
        except OSError:
            return False
        return True

    def try_reclaim_stale_lock() -> bool:
        owner = read_owner()
        if owner is not None:
            try:
                owner_pid = int(owner.get("pid", 0))
            except (TypeError, ValueError):
                owner_pid = 0
            if owner_pid > 0 and not is_process_alive(owner_pid):
                print(f"[WARN] Reclaim stale lock from dead pid {owner_pid}: {lock_dir}")
                shutil.rmtree(lock_dir, ignore_errors=True)
                return True
            return False

        try:
            age_seconds = time.time() - lock_dir.stat().st_mtime
        except OSError:
            return False
        if age_seconds >= orphan_reclaim_seconds:
            print(f"[WARN] Reclaim orphan lock (no metadata, age={age_seconds:.1f}s): {lock_dir}")
            shutil.rmtree(lock_dir, ignore_errors=True)
            return True
        return False

    while True:
        try:
            lock_dir.mkdir()
            write_owner()
            break
        except FileExistsError:
            if try_reclaim_stale_lock():
                continue
            if time.time() - start > timeout_seconds:
                owner = read_owner()
                owner_text = f", owner={owner}" if owner is not None else ""
                raise TimeoutError(f"Timed out waiting for dist lock: {lock_dir}{owner_text}") from None
            time.sleep(0.25)
    try:
        yield
    finally:
        shutil.rmtree(lock_dir, ignore_errors=True)
