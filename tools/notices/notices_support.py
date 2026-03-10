from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover
    import tomli as tomllib  # type: ignore

SCHEMA_VERSION = "1"
WINDOWS_TARGET = "windows"
ANDROID_TARGET = "android"
TARGET_CHOICES = (WINDOWS_TARGET, ANDROID_TARGET)
LAYER_TARGETS = {
    WINDOWS_TARGET: ("bills_core", "bills_io", "bills_cli"),
    ANDROID_TARGET: ("bills_core", "bills_io", "bills_android"),
}
LAYER_MANIFEST_PATHS = {
    "bills_core": Path("libs") / "bills_core" / "notices.toml",
    "bills_io": Path("libs") / "bills_io" / "notices.toml",
    "bills_cli": Path("apps") / "bills_cli" / "notices.toml",
    "bills_android": Path("apps") / "bills_android" / "notices.toml",
}


@dataclass(frozen=True)
class CatalogEntry:
    package_id: str
    display_name: str
    homepage: str
    license_spdx: str
    license_text: str
    notice_text: str | None


@dataclass(frozen=True)
class LayerDependency:
    package_id: str
    cmake_targets: tuple[str, ...]
    maven_groups: tuple[str, ...]
    maven_coordinates: tuple[str, ...]


@dataclass(frozen=True)
class LayerManifest:
    layer_id: str
    dependencies: tuple[LayerDependency, ...]


@dataclass(frozen=True)
class ResolvedArtifact:
    group: str
    name: str
    version: str
    coordinate: str


def normalize_targets(raw_targets: str | list[str] | tuple[str, ...]) -> list[str]:
    if isinstance(raw_targets, str):
        candidates = [item.strip() for item in raw_targets.split(",")]
    else:
        candidates = []
        for item in raw_targets:
            candidates.extend(part.strip() for part in item.split(","))

    normalized: list[str] = []
    for candidate in candidates:
        if not candidate:
            continue
        target = candidate.lower()
        if target not in TARGET_CHOICES:
            raise ValueError(
                f"Unsupported target '{candidate}'. Supported: {', '.join(TARGET_CHOICES)}."
            )
        if target not in normalized:
            normalized.append(target)

    if not normalized:
        raise ValueError("No valid target specified.")
    return normalized


def _load_toml(path: Path) -> dict[str, Any]:
    with path.open("rb") as handle:
        payload = tomllib.load(handle)
    if not isinstance(payload, dict):
        raise ValueError(f"TOML root must be a table in {path}.")
    return payload


def _load_utf8_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError as error:
        raise ValueError(f"Expected UTF-8 text file: {path}") from error


def _load_string_list(
    payload: dict[str, Any],
    key: str,
    source: Path,
    *,
    required: bool = False,
) -> tuple[str, ...]:
    value = payload.get(key)
    if value is None:
        if required:
            raise ValueError(f"Missing required key '{key}' in {source}.")
        return ()
    if not isinstance(value, list):
        raise ValueError(f"Key '{key}' must be an array in {source}.")

    items: list[str] = []
    for raw_item in value:
        if not isinstance(raw_item, str) or not raw_item.strip():
            raise ValueError(f"Key '{key}' must contain non-empty strings in {source}.")
        items.append(raw_item.strip())
    return tuple(items)


def load_catalog(catalog_root: Path, repo_root: Path) -> dict[str, CatalogEntry]:
    catalog_root = catalog_root.resolve()
    entries: dict[str, CatalogEntry] = {}
    for entry_path in sorted(catalog_root.glob("*.toml"), key=lambda path: path.name.lower()):
        payload = _load_toml(entry_path)
        package_id = str(payload.get("package_id", "")).strip()
        if not package_id:
            raise ValueError(f"Missing non-empty 'package_id' in {entry_path}.")
        if entry_path.stem != package_id:
            raise ValueError(
                f"Catalog filename '{entry_path.name}' must match package_id '{package_id}'."
            )
        if package_id in entries:
            raise ValueError(f"Duplicate package_id '{package_id}' in catalog root {catalog_root}.")

        display_name = str(payload.get("display_name", "")).strip()
        homepage = str(payload.get("homepage", "")).strip()
        license_spdx = str(payload.get("license_spdx", "")).strip()
        license_relpath = str(payload.get("license_text_relpath", "")).strip()
        notice_relpath = str(payload.get("notice_text_relpath", "")).strip()
        if not display_name or not homepage or not license_spdx or not license_relpath:
            raise ValueError(
                "Catalog entry must provide display_name, homepage, license_spdx, "
                f"and license_text_relpath: {entry_path}"
            )

        license_text = _load_utf8_text((repo_root / license_relpath).resolve())
        notice_text = None
        if notice_relpath:
            notice_text = _load_utf8_text((repo_root / notice_relpath).resolve())

        entries[package_id] = CatalogEntry(
            package_id=package_id,
            display_name=display_name,
            homepage=homepage,
            license_spdx=license_spdx,
            license_text=license_text,
            notice_text=notice_text,
        )

    if not entries:
        raise ValueError(f"No catalog entries found under {catalog_root}.")
    return entries


def load_layer_manifest(path: Path) -> LayerManifest:
    payload = _load_toml(path)
    layer_id = str(payload.get("layer_id", "")).strip()
    schema_version = str(payload.get("schema_version", "")).strip()
    if schema_version != SCHEMA_VERSION:
        raise ValueError(
            f"Unsupported schema_version '{schema_version}' in {path}. Expected {SCHEMA_VERSION}."
        )
    if not layer_id:
        raise ValueError(f"Missing non-empty 'layer_id' in {path}.")

    raw_dependencies = payload.get("deps")
    if not isinstance(raw_dependencies, list) or not raw_dependencies:
        raise ValueError(f"Manifest must define a non-empty [[deps]] array in {path}.")

    dependencies: list[LayerDependency] = []
    seen_package_ids: set[str] = set()
    for index, raw_dep in enumerate(raw_dependencies, start=1):
        if not isinstance(raw_dep, dict):
            raise ValueError(f"Each [[deps]] entry must be a table in {path} (entry {index}).")
        package_id = str(raw_dep.get("package_id", "")).strip()
        if not package_id:
            raise ValueError(f"Missing non-empty package_id in {path} (entry {index}).")
        if package_id in seen_package_ids:
            raise ValueError(f"Duplicate package_id '{package_id}' in {path}.")
        seen_package_ids.add(package_id)
        cmake_targets = _load_string_list(raw_dep, "cmake_targets", path)
        maven_groups = _load_string_list(raw_dep, "maven_groups", path)
        maven_coordinates = _load_string_list(raw_dep, "maven_coordinates", path)
        if not cmake_targets and not maven_groups and not maven_coordinates:
            raise ValueError(
                f"Dependency '{package_id}' in {path} must declare at least one selector."
            )
        dependencies.append(
            LayerDependency(
                package_id=package_id,
                cmake_targets=cmake_targets,
                maven_groups=maven_groups,
                maven_coordinates=maven_coordinates,
            )
        )

    return LayerManifest(layer_id=layer_id, dependencies=tuple(dependencies))


def load_all_layer_manifests(repo_root: Path) -> dict[str, LayerManifest]:
    manifests: dict[str, LayerManifest] = {}
    for layer_id, relpath in LAYER_MANIFEST_PATHS.items():
        manifest = load_layer_manifest((repo_root / relpath).resolve())
        if manifest.layer_id != layer_id:
            raise ValueError(
                f"Layer manifest {relpath} must declare layer_id '{layer_id}', "
                f"got '{manifest.layer_id}'."
            )
        manifests[layer_id] = manifest
    return manifests


def load_resolved_artifacts(path: Path) -> list[ResolvedArtifact]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise ValueError(f"Invalid JSON runtime artifact report: {path}") from error

    if not isinstance(payload, dict):
        raise ValueError(f"Runtime artifact report must be a JSON object: {path}")
    raw_artifacts = payload.get("artifacts")
    if not isinstance(raw_artifacts, list):
        raise ValueError(f"Runtime artifact report must contain an 'artifacts' array: {path}")

    artifacts: list[ResolvedArtifact] = []
    seen: set[str] = set()
    for raw_artifact in raw_artifacts:
        if not isinstance(raw_artifact, dict):
            raise ValueError(f"Artifact entries must be JSON objects in {path}.")
        group = str(raw_artifact.get("group", "")).strip()
        name = str(raw_artifact.get("name", "")).strip()
        version = str(raw_artifact.get("version", "")).strip()
        if not group or not name or not version:
            raise ValueError(f"Artifact entries must define group, name, and version in {path}.")
        coordinate = f"{group}:{name}:{version}"
        if coordinate in seen:
            continue
        seen.add(coordinate)
        artifacts.append(
            ResolvedArtifact(
                group=group,
                name=name,
                version=version,
                coordinate=coordinate,
            )
        )
    return sorted(artifacts, key=lambda item: item.coordinate)


def _match_dependency(
    dependency: LayerDependency,
    artifact: ResolvedArtifact,
) -> bool:
    if artifact.group in dependency.maven_groups:
        return True
    if f"{artifact.group}:{artifact.name}" in dependency.maven_coordinates:
        return True
    return False


def _match_android_artifacts_to_manifest(
    manifest: LayerManifest,
    artifacts: list[ResolvedArtifact],
    source_label: str,
) -> dict[str, list[str]]:
    matched: dict[str, list[str]] = {dependency.package_id: [] for dependency in manifest.dependencies}
    for artifact in artifacts:
        matching_dependencies = [
            dependency.package_id
            for dependency in manifest.dependencies
            if _match_dependency(dependency, artifact)
        ]
        if not matching_dependencies:
            raise ValueError(
                f"Artifact '{artifact.coordinate}' from {source_label} is not covered by "
                f"{manifest.layer_id} manifest."
            )
        if len(matching_dependencies) > 1:
            raise ValueError(
                f"Artifact '{artifact.coordinate}' from {source_label} matches multiple packages: "
                f"{', '.join(sorted(matching_dependencies))}."
            )
        matched[matching_dependencies[0]].append(artifact.coordinate)
    return matched


def _build_layer_packages(
    manifest: LayerManifest,
    catalog: dict[str, CatalogEntry],
    *,
    android_release_artifacts: list[ResolvedArtifact] | None = None,
    android_debug_artifacts: list[ResolvedArtifact] | None = None,
) -> list[dict[str, Any]]:
    if manifest.layer_id == "bills_android":
        if android_release_artifacts is None or android_debug_artifacts is None:
            raise ValueError(
                "Android notices generation requires both release and debug runtime artifact reports."
            )
        release_matches = _match_android_artifacts_to_manifest(
            manifest,
            android_release_artifacts,
            "releaseRuntimeClasspath",
        )
        _match_android_artifacts_to_manifest(
            manifest,
            android_debug_artifacts,
            "debugRuntimeClasspath",
        )
    else:
        release_matches = {}

    packages: list[dict[str, Any]] = []
    for dependency in manifest.dependencies:
        catalog_entry = catalog.get(dependency.package_id)
        if catalog_entry is None:
            raise ValueError(
                f"Unknown package_id '{dependency.package_id}' referenced by {manifest.layer_id}."
            )
        resolved_artifacts = sorted(
            set(dependency.cmake_targets).union(release_matches.get(dependency.package_id, ()))
        )
        if not resolved_artifacts and manifest.layer_id == "bills_android":
            continue
        packages.append(
            {
                "package_id": catalog_entry.package_id,
                "display_name": catalog_entry.display_name,
                "homepage": catalog_entry.homepage,
                "owners": [manifest.layer_id],
                "resolved_artifacts": resolved_artifacts,
                "license_spdx": catalog_entry.license_spdx,
                "license_text": catalog_entry.license_text,
                "notice_text": catalog_entry.notice_text,
            }
        )

    return sorted(packages, key=lambda item: item["display_name"].lower())


def _merge_packages(
    target_id: str,
    layer_packages: list[list[dict[str, Any]]],
) -> dict[str, Any]:
    merged: dict[str, dict[str, Any]] = {}
    for packages in layer_packages:
        for package in packages:
            target = merged.setdefault(
                package["package_id"],
                {
                    "package_id": package["package_id"],
                    "display_name": package["display_name"],
                    "homepage": package["homepage"],
                    "owners": [],
                    "resolved_artifacts": [],
                    "license_spdx": package["license_spdx"],
                    "license_text": package["license_text"],
                    "notice_text": package["notice_text"],
                },
            )
            target["owners"] = sorted(set(target["owners"]).union(package["owners"]))
            target["resolved_artifacts"] = sorted(
                set(target["resolved_artifacts"]).union(package["resolved_artifacts"])
            )
            if target["license_text"] != package["license_text"]:
                raise ValueError(
                    f"Package '{package['package_id']}' resolved to inconsistent license text."
                )
            if target["notice_text"] != package["notice_text"]:
                raise ValueError(
                    f"Package '{package['package_id']}' resolved to inconsistent notice text."
                )
    packages = sorted(merged.values(), key=lambda item: item["display_name"].lower())
    return {
        "schema_version": SCHEMA_VERSION,
        "target_id": target_id,
        "packages": packages,
    }


def _render_markdown(notices_payload: dict[str, Any]) -> str:
    lines = [
        "# Open Source Notices",
        "",
        f"Target: `{notices_payload['target_id']}`",
        "",
    ]
    packages = notices_payload.get("packages", [])
    for package in packages:
        lines.append(f"## {package['display_name']}")
        lines.append("")
        lines.append(f"- Package ID: `{package['package_id']}`")
        lines.append(f"- License: `{package['license_spdx']}`")
        lines.append(f"- Homepage: {package['homepage']}")
        lines.append(f"- Owners: {', '.join(package['owners'])}")
        if package["resolved_artifacts"]:
            lines.append(
                f"- Resolved Artifacts: {', '.join(f'`{item}`' for item in package['resolved_artifacts'])}"
            )
        lines.append("")
        lines.append("### License Text")
        lines.append("")
        lines.append("```text")
        lines.append(package["license_text"].rstrip())
        lines.append("```")
        notice_text = package.get("notice_text")
        if notice_text:
            lines.append("")
            lines.append("### Notice Text")
            lines.append("")
            lines.append("```text")
            lines.append(str(notice_text).rstrip())
            lines.append("```")
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def _write_payload(output_dir: Path, payload: dict[str, Any]) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "notices.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    (output_dir / "NOTICE.md").write_text(
        _render_markdown(payload),
        encoding="utf-8",
    )


def generate_notices_outputs(
    repo_root: Path,
    output_root: Path,
    targets: str | list[str] | tuple[str, ...],
    *,
    catalog_root: Path | None = None,
    android_release_artifacts_path: Path | None = None,
    android_debug_artifacts_path: Path | None = None,
) -> dict[str, Path]:
    repo_root = repo_root.resolve()
    output_root = output_root.resolve()
    normalized_targets = normalize_targets(targets)
    catalog = load_catalog(catalog_root or (repo_root / "third_party" / "catalog"), repo_root)
    manifests = load_all_layer_manifests(repo_root)

    android_release_artifacts = None
    android_debug_artifacts = None
    if ANDROID_TARGET in normalized_targets or "bills_android" in {
        layer_id
        for target in normalized_targets
        for layer_id in LAYER_TARGETS[target]
    }:
        if android_release_artifacts_path is None or android_debug_artifacts_path is None:
            raise ValueError(
                "Android notices generation requires both release and debug runtime artifact paths."
            )
        android_release_artifacts = load_resolved_artifacts(android_release_artifacts_path)
        android_debug_artifacts = load_resolved_artifacts(android_debug_artifacts_path)

    generated_outputs: dict[str, Path] = {}
    layer_package_cache: dict[str, list[dict[str, Any]]] = {}
    required_layers = {
        layer_id
        for target in normalized_targets
        for layer_id in LAYER_TARGETS[target]
    }

    for layer_id in sorted(required_layers):
        packages = _build_layer_packages(
            manifests[layer_id],
            catalog,
            android_release_artifacts=android_release_artifacts,
            android_debug_artifacts=android_debug_artifacts,
        )
        layer_package_cache[layer_id] = packages
        layer_payload = {
            "schema_version": SCHEMA_VERSION,
            "target_id": layer_id,
            "packages": packages,
        }
        layer_output_dir = output_root / "layers" / layer_id
        _write_payload(layer_output_dir, layer_payload)
        generated_outputs[layer_id] = layer_output_dir

    for target in normalized_targets:
        payload = _merge_packages(
            target,
            [layer_package_cache[layer_id] for layer_id in LAYER_TARGETS[target]],
        )
        target_output_dir = output_root / target
        _write_payload(target_output_dir, payload)
        generated_outputs[target] = target_output_dir

    return generated_outputs
