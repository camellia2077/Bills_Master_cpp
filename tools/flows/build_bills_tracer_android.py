#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

try:
    from ._bootstrap import bootstrap_repo_root
except ImportError:
    from _bootstrap import bootstrap_repo_root

REPO_ROOT = bootstrap_repo_root(__file__)
ANDROID_PROJECT_DIR = REPO_ROOT / "apps" / "bills_android"
ANDROID_BUILD_FILE = ANDROID_PROJECT_DIR / "build.gradle.kts"
GRADLEW = REPO_ROOT / ("gradlew.bat" if sys.platform.startswith("win") else "gradlew")
DIST_ANDROID_ROOT = REPO_ROOT / "dist" / "gradle" / "apps" / "bills_android"
DIST_APK_ROOT = DIST_ANDROID_ROOT / "apk"
DIST_SIGNING_ROOT = DIST_ANDROID_ROOT / "signing"
DIST_NATIVE_DEPS_ROOT = DIST_ANDROID_ROOT / "native_deps"
SUPPORTED_VARIANTS = ("debug", "release")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build Android APKs for apps/bills_android via Gradle wrapper."
    )
    variant_group = parser.add_mutually_exclusive_group()
    variant_group.add_argument(
        "--preset",
        choices=list(SUPPORTED_VARIANTS),
        help="Build one variant via the unified dist interface.",
    )
    variant_group.add_argument(
        "--variants",
        help="Comma-separated variants to build. Defaults to debug.",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Run Gradle clean before assembling APKs.",
    )
    parser.add_argument(
        "--skip_copy",
        action="store_true",
        help="Do not mirror built APKs into dist/gradle/apps/bills_android/apk/.",
    )
    return parser.parse_args(argv)


def normalize_variants(raw_value: str) -> list[str]:
    mapping = {variant: variant for variant in SUPPORTED_VARIANTS}
    variants: list[str] = []
    for item in raw_value.split(","):
        normalized = mapping.get(item.strip().lower())
        if normalized is None:
            raise ValueError(f"Unsupported variant: {item.strip()!r}")
        if normalized not in variants:
            variants.append(normalized)
    if not variants:
        raise ValueError("At least one Android build variant is required.")
    return variants


def resolve_requested_variants(args: argparse.Namespace) -> list[str]:
    if args.variants:
        return normalize_variants(str(args.variants))
    if args.preset:
        return [str(args.preset).strip().lower()]
    return ["debug"]


def read_android_version_name() -> str:
    marker = 'val androidPresentationVersionName = "'
    for line in ANDROID_BUILD_FILE.read_text(encoding="utf-8").splitlines():
        if marker in line:
            return line.split(marker, 1)[1].split('"', 1)[0].strip()
    raise RuntimeError("Failed to read androidPresentationVersionName from build.gradle.kts.")


def run_command(command: list[str], *, cwd: Path) -> None:
    print(f"==> Running: {' '.join(str(part) for part in command)}")
    subprocess.run(command, cwd=cwd, check=True)


def iter_dependency_source_candidates() -> list[Path]:
    search_roots = [
        DIST_NATIVE_DEPS_ROOT,
        ANDROID_PROJECT_DIR / ".cxx",
        REPO_ROOT / "dist" / "cmake",
    ]
    candidates: list[Path] = []
    for search_root in search_roots:
        if not search_root.exists():
            continue
        candidates.extend(path for path in search_root.rglob("*") if path.is_dir())
    return candidates


def resolve_dependency_source_dir(dependency_name: str) -> Path | None:
    validators = {
        "nlohmann_json-src": lambda path: (path / "CMakeLists.txt").is_file() and (path / "include" / "nlohmann" / "json.hpp").is_file(),
        "tomlplusplus-src": lambda path: (path / "CMakeLists.txt").is_file() and (path / "include" / "toml++" / "toml.hpp").is_file(),
        "sqlite_amalgamation-src": lambda path: (path / "sqlite3.c").is_file() and (path / "sqlite3.h").is_file(),
    }
    validate = validators[dependency_name]
    matches = [path for path in iter_dependency_source_candidates() if path.name == dependency_name and validate(path)]
    if not matches:
        return None
    matches.sort(key=lambda path: len(path.parts))
    return matches[0]


def stage_native_dependencies() -> dict[str, str]:
    dependency_map = {
        "BILLS_ANDROID_NLOHMANN_JSON_SOURCE_DIR": ("nlohmann_json-src", "nlohmann_json"),
        "BILLS_ANDROID_TOMLPLUSPLUS_SOURCE_DIR": ("tomlplusplus-src", "tomlplusplus"),
        "BILLS_ANDROID_SQLITE_AMALGAMATION_SOURCE_DIR": ("sqlite_amalgamation-src", "sqlite_amalgamation"),
    }
    staged_properties: dict[str, str] = {}
    DIST_NATIVE_DEPS_ROOT.mkdir(parents=True, exist_ok=True)

    for property_name, (source_dir_name, target_dir_name) in dependency_map.items():
        source_dir = resolve_dependency_source_dir(source_dir_name)
        target_dir = DIST_NATIVE_DEPS_ROOT / target_dir_name
        if source_dir is None and target_dir.exists():
            source_dir = target_dir
        if source_dir is None:
            print(f"==> No local cache found for {source_dir_name}; Gradle/CMake may download it during the build.")
            continue
        if source_dir.resolve() != target_dir.resolve() and target_dir.exists():
            shutil.rmtree(target_dir)
        if source_dir.resolve() != target_dir.resolve():
            shutil.copytree(source_dir, target_dir)
        staged_properties[property_name] = target_dir.resolve().as_posix()
        print(f"==> Reusing local native dependency cache for {source_dir_name}: {target_dir}")

    return staged_properties


def resolve_keytool() -> Path:
    which_command = ["where.exe", "keytool"] if sys.platform.startswith("win") else ["which", "keytool"]
    result = subprocess.run(
        which_command,
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    candidates = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if not candidates:
        raise FileNotFoundError("Unable to find keytool in PATH.")
    return Path(candidates[0])


def ensure_release_keystore() -> dict[str, str]:
    DIST_SIGNING_ROOT.mkdir(parents=True, exist_ok=True)
    keystore_path = DIST_SIGNING_ROOT / "bills-android-release.keystore"
    signing = {
        "store_file": str(keystore_path),
        "store_password": "bills-android-release",
        "key_alias": "bills-android",
        "key_password": "bills-android-release",
    }
    if keystore_path.exists():
        return signing

    keytool = resolve_keytool()
    command = [
        str(keytool),
        "-genkeypair",
        "-keystore",
        str(keystore_path),
        "-storepass",
        signing["store_password"],
        "-keypass",
        signing["key_password"],
        "-alias",
        signing["key_alias"],
        "-keyalg",
        "RSA",
        "-keysize",
        "2048",
        "-validity",
        "36500",
        "-dname",
        "CN=BillsTracer, OU=Personal, O=BillsTracer, L=Shanghai, ST=Shanghai, C=CN",
        "-noprompt",
    ]
    run_command(command, cwd=REPO_ROOT)
    return signing


def gradle_properties_for_release(signing: dict[str, str]) -> list[str]:
    return [
        f"-PBILLS_ANDROID_RELEASE_STORE_FILE={signing['store_file']}",
        f"-PBILLS_ANDROID_RELEASE_STORE_PASSWORD={signing['store_password']}",
        f"-PBILLS_ANDROID_RELEASE_KEY_ALIAS={signing['key_alias']}",
        f"-PBILLS_ANDROID_RELEASE_KEY_PASSWORD={signing['key_password']}",
    ]


def assemble_variants(variants: list[str], *, clean: bool) -> None:
    version_name = read_android_version_name()
    release_signing = ensure_release_keystore() if "release" in variants else None
    native_dependency_properties = stage_native_dependencies()

    tasks: list[str] = []
    if clean:
        tasks.append("clean")
    for variant in variants:
        tasks.append(f":apps:bills_android:assemble{variant.capitalize()}")

    command = [str(GRADLEW), *tasks]
    if release_signing is not None:
        command.extend(gradle_properties_for_release(release_signing))
    for property_name, property_value in native_dependency_properties.items():
        command.append(f"-P{property_name}={property_value}")
    run_command(command, cwd=REPO_ROOT)

    print("==> Build finished.")
    for variant in variants:
        apk_path = resolve_apk_path(variant)
        print(f"   {variant}: {apk_path}")
    print(f"   version: {version_name}")


def resolve_apk_path(variant: str) -> Path:
    apk_path = (
        ANDROID_PROJECT_DIR
        / "build"
        / "outputs"
        / "apk"
        / variant
        / f"bills_android-{variant}.apk"
    )
    if not apk_path.is_file():
        raise FileNotFoundError(f"Expected APK was not produced: {apk_path}")
    return apk_path


def copy_apks_to_dist(variants: list[str]) -> None:
    version_name = read_android_version_name()
    for variant in variants:
        source_apk = resolve_apk_path(variant)
        target_dir = DIST_APK_ROOT / variant
        target_dir.mkdir(parents=True, exist_ok=True)
        target_apk = target_dir / f"bills_android-{variant}-v{version_name}.apk"
        shutil.copy2(source_apk, target_apk)
        print(f"==> Copied {variant} APK to {target_apk}")


def main() -> int:
    args = parse_args()
    try:
        variants = resolve_requested_variants(args)
        assemble_variants(variants, clean=args.clean)
        if not args.skip_copy:
            copy_apks_to_dist(variants)
    except (subprocess.CalledProcessError, FileNotFoundError, RuntimeError, ValueError) as exc:
        print(f"Android build failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
