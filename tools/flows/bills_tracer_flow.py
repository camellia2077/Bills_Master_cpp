#!/usr/bin/env python3
"""Build apps/bills_cli and then run CLI tests from tests/."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path


FORMAT_CONFIG = {
    "json": {
        "cmake_option": "",
    },
    "md": {
        "cmake_option": "ENABLE_FMT_MD",
    },
    "rst": {
        "cmake_option": "ENABLE_FMT_RST",
    },
    "tex": {
        "cmake_option": "ENABLE_FMT_TEX",
    },
}
RUNTIME_EXPORT_FORMATS_FILENAME = "export_formats.toml"
TEST_SUMMARY_FILENAME = "test_summary.json"
PYTHON_TEST_LOG_FILENAME = "test_python_output.log"
RUN_MANIFEST_FILENAME = "run_manifest.json"
DEFAULT_OUTPUT_PROJECT = "bills_tracer"
DEFAULT_OUTPUT_GROUP = "artifact"
RUNTIME_OUTPUT_GROUP = "runtime"
DEFAULT_BUILD_ROOT = "apps/bills_cli/build_cli_test"
DEFAULT_BUILD_DIR_MODE = "isolated"
DEFAULT_MAX_RUNS = 20
RUNS_DIR_NAME = "runs"
LATEST_DIR_NAME = "latest"
LATEST_SYNC_LOCK_DIR_NAME = ".latest_sync.lock"
LOCK_OWNER_FILENAME = ".lock_owner.json"
ORPHAN_LOCK_RECLAIM_SECONDS = 120

CLEANUP_FILES = ["bills.sqlite3"]
CLEANUP_DIRS = ["build", "config", "output"]
RUNTIME_SIDECAR_EXTS = {".dll", ".exe", ".manifest", ".pdb"}


def run_command(command: list[str], cwd: Path | None = None,
                env: dict[str, str] | None = None) -> None:
    print(f"==> {' '.join(command)}")
    subprocess.run(command, cwd=str(cwd) if cwd else None, env=env, check=True)


def sanitize_segment(value: str) -> str:
    allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
    normalized = "".join(ch if ch in allowed else "_" for ch in value.strip())
    normalized = normalized.strip("_")
    return normalized or "default"


def short_hash(text: str, length: int = 8) -> str:
    return hashlib.sha1(text.encode("utf-8")).hexdigest()[:length]


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
            # Lock ownership metadata is best-effort.
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
                print(
                    f"[WARN] Reclaim stale lock from dead pid {owner_pid}: {lock_dir}"
                )
                shutil.rmtree(lock_dir, ignore_errors=True)
                return True
            return False

        # Backward compatibility for old lock dirs without owner metadata.
        try:
            age_seconds = time.time() - lock_dir.stat().st_mtime
        except OSError:
            return False
        if age_seconds >= orphan_reclaim_seconds:
            print(
                f"[WARN] Reclaim orphan lock (no metadata, age={age_seconds:.1f}s): "
                f"{lock_dir}"
            )
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
                raise TimeoutError(
                    f"Timed out waiting for build lock: {lock_dir}{owner_text}"
                )
            time.sleep(0.25)
    try:
        yield
    finally:
        shutil.rmtree(lock_dir, ignore_errors=True)


def load_test_summary(summary_path: Path) -> dict | None:
    if not summary_path.exists():
        return None
    try:
        return json.loads(summary_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def write_python_test_log(
    log_path: Path,
    command: list[str],
    started_at: datetime,
    completed_at: datetime,
    elapsed_seconds: float,
    return_code: int,
    stdout: str,
    stderr: str,
) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        f"started_at={started_at.isoformat(timespec='microseconds')}",
        f"completed_at={completed_at.isoformat(timespec='microseconds')}",
        f"elapsed_seconds={elapsed_seconds:.6f}",
        f"return_code={return_code}",
        f"command={' '.join(command)}",
        "",
        "stdout:",
        stdout.rstrip(),
        "",
        "stderr:",
        stderr.rstrip(),
        "",
    ]
    log_path.write_text("\n".join(lines), encoding="utf-8")


def read_cache_home_directory(cache_file: Path) -> Path | None:
    try:
        with cache_file.open("r", encoding="utf-8", errors="ignore") as handle:
            for line in handle:
                if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
                    return Path(line.split("=", 1)[1].strip())
    except OSError:
        return None
    return None


def read_cache_compilers(cache_file: Path) -> tuple[str | None, str | None]:
    c_compiler = None
    cxx_compiler = None
    try:
        with cache_file.open("r", encoding="utf-8", errors="ignore") as handle:
            for line in handle:
                if line.startswith("CMAKE_C_COMPILER:FILEPATH="):
                    c_compiler = line.split("=", 1)[1].strip()
                elif line.startswith("CMAKE_CXX_COMPILER:FILEPATH="):
                    cxx_compiler = line.split("=", 1)[1].strip()
    except OSError:
        return None, None
    return c_compiler, cxx_compiler


def cache_uses_clang(cache_file: Path) -> bool:
    c_compiler, cxx_compiler = read_cache_compilers(cache_file)
    if not c_compiler or not cxx_compiler:
        return False
    c_name = Path(c_compiler).name.lower()
    cxx_name = Path(cxx_compiler).name.lower()
    return "clang" in c_name and "clang" in cxx_name


def prepare_build_dir(build_dir: Path, source_dir: Path) -> None:
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.exists():
        return
    cached_home = read_cache_home_directory(cache_file)
    if (
        cached_home is not None
        and cached_home.resolve() == source_dir.resolve()
        and cache_uses_clang(cache_file)
    ):
        return
    shutil.rmtree(build_dir)


def resolve_build_dir(
    repo_root: Path,
    explicit_build_dir: str | None,
    build_dir_mode: str,
    output_project: str,
    export_pipeline: str,
    formats: list[str],
    build_type: str,
    generator: str,
) -> Path:
    if explicit_build_dir:
        return (repo_root / explicit_build_dir).resolve()

    build_root = (repo_root / DEFAULT_BUILD_ROOT).resolve()
    if build_dir_mode == "shared":
        return (build_root / "shared").resolve()

    format_tag = sanitize_segment("-".join(formats))
    project_tag = sanitize_segment(output_project)
    pipeline_tag = sanitize_segment(export_pipeline)
    build_type_tag = sanitize_segment(build_type.lower())
    generator_tag = sanitize_segment(generator.lower())
    unique_seed = f"{project_tag}|{pipeline_tag}|{format_tag}|{build_type_tag}|{generator_tag}"
    unique_tag = short_hash(unique_seed, length=12)
    build_name = f"b_{unique_tag}"
    return (build_root / "iso" / build_name).resolve()


def make_runtime_base_dir(
    runtime_project_root: Path,
    output_project: str,
    export_pipeline: str,
    run_tag: str,
) -> Path:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    project_tag = sanitize_segment(output_project)
    pipeline_tag = sanitize_segment(export_pipeline)
    custom_tag = sanitize_segment(run_tag) if run_tag else "auto"
    # Keep runtime directory names short to avoid Windows MAX_PATH issues
    # during nested export path creation in parallel smoke workflows.
    project_short = project_tag[:12]
    pipeline_short = pipeline_tag[:10]
    custom_short = custom_tag[:10]
    fingerprint = short_hash(
        f"{project_tag}|{pipeline_tag}|{custom_tag}|{os.getpid()}",
        length=8,
    )
    folder = (
        f"{timestamp}_{project_short}_{pipeline_short}_{custom_short}_{fingerprint}"
    )
    runtime_base = runtime_project_root / RUNS_DIR_NAME / folder
    runtime_base.mkdir(parents=True, exist_ok=True)
    return runtime_base.resolve()


def make_run_output_dir(project_output_root: Path, run_id: str) -> Path:
    run_output_dir = project_output_root / RUNS_DIR_NAME / run_id
    run_output_dir.mkdir(parents=True, exist_ok=True)
    return run_output_dir.resolve()


def write_json_file(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )


def replace_path(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        if dst.is_dir():
            shutil.rmtree(dst)
        else:
            dst.unlink()
    if src.is_dir():
        shutil.copytree(src, dst)
    else:
        shutil.copy2(src, dst)


def sync_runtime_workspace(build_bin_dir: Path, workspace_dir: Path) -> None:
    if not build_bin_dir.exists():
        raise FileNotFoundError(
            f"Build output directory not found: {build_bin_dir}"
        )

    workspace_dir.mkdir(parents=True, exist_ok=True)
    for stale in workspace_dir.iterdir():
        if stale.is_file() and stale.suffix.lower() in RUNTIME_SIDECAR_EXTS:
            stale.unlink()

    copied_files: list[str] = []
    for entry in sorted(build_bin_dir.iterdir(), key=lambda p: p.name.lower()):
        if not entry.is_file():
            continue
        if entry.suffix.lower() not in RUNTIME_SIDECAR_EXTS:
            continue
        shutil.copy2(entry, workspace_dir / entry.name)
        copied_files.append(entry.name)

    replace_path(build_bin_dir / "config", workspace_dir / "config")
    print(
        "==> Synced runtime artifacts to "
        f"{workspace_dir} ({len(copied_files)} files + config)"
    )


def sync_latest_project_outputs(project_output_root: Path, run_output_dir: Path) -> None:
    lock_dir = project_output_root / LATEST_SYNC_LOCK_DIR_NAME
    with directory_lock(lock_dir):
        latest_output_root = project_output_root / LATEST_DIR_NAME
        if latest_output_root.exists():
            shutil.rmtree(latest_output_root, ignore_errors=True)
        latest_output_root.mkdir(parents=True, exist_ok=True)
        for legacy_name in [
            TEST_SUMMARY_FILENAME,
            PYTHON_TEST_LOG_FILENAME,
            RUN_MANIFEST_FILENAME,
            "logs",
            "txt2josn",
            "exported_files",
        ]:
            legacy_path = project_output_root / legacy_name
            if not legacy_path.exists():
                continue
            if legacy_path.is_dir():
                shutil.rmtree(legacy_path, ignore_errors=True)
            else:
                legacy_path.unlink(missing_ok=True)
        replace_path(
            run_output_dir / TEST_SUMMARY_FILENAME,
            latest_output_root / TEST_SUMMARY_FILENAME,
        )
        replace_path(
            run_output_dir / PYTHON_TEST_LOG_FILENAME,
            latest_output_root / PYTHON_TEST_LOG_FILENAME,
        )
        replace_path(
            run_output_dir / RUN_MANIFEST_FILENAME,
            latest_output_root / RUN_MANIFEST_FILENAME,
        )
        replace_path(run_output_dir / "logs", latest_output_root / "logs")
        replace_path(run_output_dir / "txt2josn", latest_output_root / "txt2josn")
        replace_path(
            run_output_dir / "exported_files",
            latest_output_root / "exported_files",
        )
        (project_output_root / "latest_run.txt").write_text(
            run_output_dir.name,
            encoding="utf-8",
        )


def prune_old_runs(project_output_root: Path, max_runs: int) -> None:
    if max_runs <= 0:
        return
    runs_root = project_output_root / RUNS_DIR_NAME
    if not runs_root.exists():
        return
    run_dirs = [path for path in runs_root.iterdir() if path.is_dir()]
    if len(run_dirs) <= max_runs:
        return
    run_dirs.sort(key=lambda path: path.stat().st_mtime, reverse=True)
    for old_run_dir in run_dirs[max_runs:]:
        shutil.rmtree(old_run_dir, ignore_errors=True)


def parse_formats(raw_formats: str) -> list[str]:
    formats = [item.strip().lower() for item in raw_formats.split(",") if item.strip()]
    if not formats:
        raise ValueError("No valid format found in --formats.")
    unsupported = [fmt for fmt in formats if fmt not in FORMAT_CONFIG]
    if unsupported:
        raise ValueError(
            f"Unsupported format(s): {', '.join(unsupported)}. "
            f"Supported: {', '.join(FORMAT_CONFIG.keys())}."
        )
    deduped: list[str] = []
    for fmt in formats:
        if fmt not in deduped:
            deduped.append(fmt)
    return deduped


def cmake_format_defines(formats: list[str]) -> list[str]:
    enabled = set(formats)
    defines: list[str] = []
    for fmt, cfg in FORMAT_CONFIG.items():
        option = cfg["cmake_option"]
        if not option:
            continue
        defines.append(f"-D{option}={'ON' if fmt in enabled else 'OFF'}")
    return defines


def write_runtime_export_formats_config(
    build_bin_dir: Path, export_formats: list[str]
) -> None:
    config_dir = build_bin_dir / "config"
    config_dir.mkdir(parents=True, exist_ok=True)
    config_path = config_dir / RUNTIME_EXPORT_FORMATS_FILENAME
    config_path.write_text(
        f"enabled_formats = {to_toml_list(export_formats)}\n",
        encoding="utf-8",
    )


def to_toml_list(items: list[str]) -> str:
    return "[" + ", ".join(f"'{item}'" for item in items) + "]"


def build_cleanup_dirs() -> list[str]:
    return [*CLEANUP_DIRS]


def write_temp_test_config(
    config_path: Path,
    build_bin_dir: Path,
    bills_dir: Path,
    import_dir: Path,
    runtime_base_dir: Path,
    runtime_run_id: str,
    runtime_output_dir: Path,
    runtime_summary_path: Path,
    run_export_all_tasks: bool,
    export_formats: list[str],
    ingest_mode: str,
    ingest_write_json: bool,
    export_pipeline: str,
    output_project: str,
    single_year: str,
    single_month: str,
    range_start: str,
    range_end: str,
) -> None:
    config_text = f"""[paths]
build_dir = '{build_bin_dir.as_posix()}'
bills_dir = '{bills_dir.as_posix()}'
import_dir = '{import_dir.as_posix()}'

[runtime]
base_dir = '{runtime_base_dir.as_posix()}'
run_id = '{runtime_run_id}'
output_dir = '{runtime_output_dir.as_posix()}'
summary_path = '{runtime_summary_path.as_posix()}'

[run_control]
run_cleanup = true
run_prepare_env = true
run_tests = true

[settings]
run_export_all_tasks = {str(run_export_all_tasks).lower()}
ingest_mode = '{ingest_mode}'
ingest_write_json = {str(ingest_write_json).lower()}
export_pipeline = '{export_pipeline}'
export_formats = {to_toml_list(export_formats)}
output_project = '{output_project}'

[cleanup]
files_to_delete = {to_toml_list(CLEANUP_FILES)}
dirs_to_delete = {to_toml_list(build_cleanup_dirs())}

[test_dates]
single_year = '{single_year}'
single_month = '{single_month}'
range_start = '{range_start}'
range_end = '{range_end}'
"""
    config_path.write_text(config_text, encoding="utf-8")


def build_cli(source_dir: Path, build_dir: Path, generator: str,
              build_type: str, target: str,
              cmake_defines: list[str]) -> None:
    lock_dir = build_dir.parent / f"{build_dir.name}.lock"
    with directory_lock(lock_dir):
        prepare_build_dir(build_dir, source_dir)
        configure_cmd = [
            "cmake",
            "-S",
            str(source_dir),
            "-B",
            str(build_dir),
            "-G",
            generator,
            f"-DCMAKE_BUILD_TYPE={build_type}",
            "-DBILL_COMPILER=clang",
            "-DBILLS_CORE_BUILD_SHARED=ON",
            *cmake_defines,
        ]
        run_command(configure_cmd)

        build_cmd = ["cmake", "--build", str(build_dir), "--target", target]
        run_command(build_cmd)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build bill_master_cli and run command-line tests."
    )
    parser.add_argument("--build-dir", default="")
    parser.add_argument(
        "--build-dir-mode",
        default=DEFAULT_BUILD_DIR_MODE,
        choices=["isolated", "shared"],
        help="isolated=dedicated build dir per test run; shared=single reusable build dir.",
    )
    parser.add_argument("--build-type", default="Debug")
    parser.add_argument("--generator", default="Ninja")
    parser.add_argument("--target", default="bill_master_cli")
    parser.add_argument("--bills-dir", default="tests/fixtures/bills")
    parser.add_argument("--formats", default="md")
    parser.add_argument("--ingest-mode", default="stepwise",
                        choices=["stepwise", "ingest"])
    parser.add_argument("--ingest-write-json", action="store_true")
    parser.add_argument("--run-export-all", action="store_true")
    parser.add_argument(
        "--export-pipeline",
        default="model-first",
        choices=["legacy", "model-first", "json-first"],
    )
    parser.add_argument("--single-year", default="2025")
    parser.add_argument("--single-month", default="2025-01")
    parser.add_argument("--range-start", default="2025-03")
    parser.add_argument("--range-end", default="2025-04")
    parser.add_argument("--python", default=sys.executable)
    parser.add_argument("--output-project", default=DEFAULT_OUTPUT_PROJECT)
    parser.add_argument(
        "--run-tag",
        default="",
        help="Optional label for runtime workspace folder naming.",
    )
    parser.add_argument(
        "--max-runs",
        type=int,
        default=DEFAULT_MAX_RUNS,
        help="Maximum number of historical run artifacts to keep per output project. <=0 disables pruning.",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    source_dir = repo_root / "apps" / "bills_cli"
    export_formats = parse_formats(args.formats)
    output_project_name = sanitize_segment(args.output_project)
    explicit_build_dir = args.build_dir.strip()
    build_dir = resolve_build_dir(
        repo_root=repo_root,
        explicit_build_dir=explicit_build_dir if explicit_build_dir else None,
        build_dir_mode=args.build_dir_mode,
        output_project=output_project_name,
        export_pipeline=args.export_pipeline,
        formats=export_formats,
        build_type=args.build_type,
        generator=args.generator,
    )
    build_bin_dir = build_dir / "bin"
    bills_dir = repo_root / args.bills_dir
    test_root = repo_root / "tests"
    project_output_root = (
        test_root / "output" / DEFAULT_OUTPUT_GROUP / output_project_name
    )
    runtime_project_root = (
        test_root / "output" / RUNTIME_OUTPUT_GROUP / output_project_name
    )
    runtime_workspace_dir = (runtime_project_root / "workspace").resolve()
    test_runner = test_root / "suites" / "artifact" / "bills_master" / "run_tests.py"
    runtime_base_dir = make_runtime_base_dir(
        runtime_project_root=runtime_project_root,
        output_project=output_project_name,
        export_pipeline=args.export_pipeline,
        run_tag=args.run_tag,
    )
    run_id = runtime_base_dir.name
    run_output_dir = make_run_output_dir(project_output_root, run_id)
    test_workdir = runtime_base_dir
    import_dir = runtime_base_dir / "output" / "txt2josn"
    summary_path = run_output_dir / TEST_SUMMARY_FILENAME
    python_test_log_path = run_output_dir / PYTHON_TEST_LOG_FILENAME

    if not bills_dir.exists():
        print(f"Error: bills data directory not found: {bills_dir}")
        print(
            "Hint: generate data first (e.g. via tests/generators/log_generator)."
        )
        return 2
    if not test_runner.exists():
        print(f"Error: test runner not found: {test_runner}")
        return 2

    format_defines = cmake_format_defines(export_formats)

    try:
        build_cli(
            source_dir=source_dir,
            build_dir=build_dir,
            generator=args.generator,
            build_type=args.build_type,
            target=args.target,
            cmake_defines=format_defines,
        )
        sync_runtime_workspace(build_bin_dir, runtime_workspace_dir)
        write_runtime_export_formats_config(runtime_workspace_dir, export_formats)
    except subprocess.CalledProcessError as exc:
        print(f"Build failed with exit code {exc.returncode}.")
        return exc.returncode
    except FileNotFoundError as exc:
        print(f"Build failed: {exc}")
        return 2

    temp_dir = repo_root / "temp"
    temp_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w",
        suffix=".toml",
        prefix="run_command_",
        dir=temp_dir,
        delete=False,
        encoding="utf-8",
    ) as tmp_file:
        temp_config_path = Path(tmp_file.name)

    try:
        write_json_file(
            run_output_dir / "run_manifest.json",
            {
                "run_id": run_id,
                "status": "running",
                "output_project": output_project_name,
                "export_pipeline": args.export_pipeline,
                "formats": export_formats,
                "build_dir": build_dir.as_posix(),
                "runtime_workspace_dir": runtime_workspace_dir.as_posix(),
                "runtime_base_dir": runtime_base_dir.as_posix(),
                "run_output_dir": run_output_dir.as_posix(),
                "created_at": datetime.now().isoformat(timespec="seconds"),
            },
        )
        write_temp_test_config(
            config_path=temp_config_path,
            build_bin_dir=runtime_workspace_dir,
            bills_dir=bills_dir,
            import_dir=import_dir,
            runtime_base_dir=runtime_base_dir,
            runtime_run_id=run_id,
            runtime_output_dir=run_output_dir,
            runtime_summary_path=summary_path,
            run_export_all_tasks=args.run_export_all,
            export_formats=export_formats,
            ingest_mode=args.ingest_mode,
            ingest_write_json=args.ingest_write_json,
            export_pipeline=args.export_pipeline,
            output_project=output_project_name,
            single_year=args.single_year,
            single_month=args.single_month,
            range_start=args.range_start,
            range_end=args.range_end,
        )

        env = os.environ.copy()
        env["BILLS_MASTER_TEST_CONFIG"] = str(temp_config_path)
        print(f"==> {args.python} {test_runner}")
        test_cmd = [args.python, str(test_runner)]
        started_at = datetime.now()
        started_perf = time.perf_counter()
        test_result = subprocess.run(
            test_cmd,
            cwd=str(test_workdir),
            env=env,
            capture_output=True,
            text=True,
            check=False,
        )
        elapsed_seconds = time.perf_counter() - started_perf
        completed_at = datetime.now()
        print(test_result.stdout, end="")
        if test_result.stderr:
            print(test_result.stderr, end="", file=sys.stderr)
        write_python_test_log(
            log_path=python_test_log_path,
            command=test_cmd,
            started_at=started_at,
            completed_at=completed_at,
            elapsed_seconds=elapsed_seconds,
            return_code=test_result.returncode,
            stdout=test_result.stdout,
            stderr=test_result.stderr,
        )
        print(f"CLI python output log: {python_test_log_path}")
        summary = load_test_summary(summary_path)
        if summary is None:
            print(f"CLI tests did not produce summary JSON: {summary_path}")
            write_json_file(
                run_output_dir / "run_manifest.json",
                {
                    "run_id": run_id,
                    "status": "failed_no_summary",
                    "output_project": output_project_name,
                    "export_pipeline": args.export_pipeline,
                    "formats": export_formats,
                    "build_dir": build_dir.as_posix(),
                    "runtime_workspace_dir": runtime_workspace_dir.as_posix(),
                    "runtime_base_dir": runtime_base_dir.as_posix(),
                    "run_output_dir": run_output_dir.as_posix(),
                    "completed_at": datetime.now().isoformat(timespec="seconds"),
                    "test_return_code": test_result.returncode,
                },
            )
            return test_result.returncode if test_result.returncode != 0 else 3
        success = int(summary.get("success", 0))
        failed = int(summary.get("failed", 0))
        total = int(summary.get("total", 0))
        ok = bool(summary.get("ok", False))
        print(
            "CLI test summary: "
            f"ok={ok}, total={total}, success={success}, failed={failed}"
        )
        write_json_file(
            run_output_dir / "run_manifest.json",
            {
                "run_id": run_id,
                "status": "ok" if ok and failed == 0 else "failed",
                "output_project": output_project_name,
                "export_pipeline": args.export_pipeline,
                "formats": export_formats,
                "build_dir": build_dir.as_posix(),
                "runtime_workspace_dir": runtime_workspace_dir.as_posix(),
                "runtime_base_dir": runtime_base_dir.as_posix(),
                "run_output_dir": run_output_dir.as_posix(),
                "completed_at": datetime.now().isoformat(timespec="seconds"),
                "test_return_code": test_result.returncode,
                "summary": summary,
            },
        )
        sync_latest_project_outputs(project_output_root, run_output_dir)
        prune_old_runs(project_output_root, args.max_runs)
        prune_old_runs(runtime_project_root, args.max_runs)
        if not ok or failed > 0:
            return test_result.returncode if test_result.returncode != 0 else 1
    except subprocess.CalledProcessError as exc:
        print(f"CLI tests failed with exit code {exc.returncode}.")
        write_json_file(
            run_output_dir / "run_manifest.json",
            {
                "run_id": run_id,
                "status": "build_or_test_crashed",
                "output_project": output_project_name,
                "export_pipeline": args.export_pipeline,
                "formats": export_formats,
                "build_dir": build_dir.as_posix(),
                "runtime_workspace_dir": runtime_workspace_dir.as_posix(),
                "runtime_base_dir": runtime_base_dir.as_posix(),
                "run_output_dir": run_output_dir.as_posix(),
                "completed_at": datetime.now().isoformat(timespec="seconds"),
                "return_code": exc.returncode,
            },
        )
        return exc.returncode
    finally:
        temp_config_path.unlink(missing_ok=True)

    print("Build and CLI tests completed successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
